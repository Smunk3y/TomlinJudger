import cv2
import numpy as np
import time
import requests
from keras.models import load_model
import serial

# Serial communication setup
ser = serial.Serial('COM6', 9600, timeout=1)
ser.flush()

# Load the model and labels
model = load_model("keras_Model.h5", compile=False)
class_names = [line.strip() for line in open("labels.txt", "r").readlines()]

# Function to fetch team information
def get_team_info(class_name):
    base_url = "https://site.api.espn.com/apis/site/v2/sports/football/nfl/teams/"
    url = base_url + class_name

    try:
        response = requests.get(url)
        response.raise_for_status()
        data = response.json()

        team_location = data.get("team", {}).get("location", "No location found")
        team_record_items = data.get("team", {}).get("record", {}).get("items", [])
        team_record = team_record_items[0].get("summary", "No record found") if team_record_items else "No record items"
        win_percentage = points_for = points_against = point_differential = 0

        if team_record_items:
            stats = team_record_items[0].get("stats", [])
            for stat in stats:
                stat_name = stat.get("name")
                if stat_name == "winPercent":
                    win_percentage = stat.get("value", 0)
                elif stat_name == "pointsFor":
                    points_for = stat.get("value", 0)
                elif stat_name == "pointsAgainst":
                    points_against = stat.get("value", 0)
                elif stat_name == "pointDifferential":
                    point_differential = stat.get("value", 0)

        return {
            "Location": team_location,
            "Record": team_record,
            "Win Percentage": f"{win_percentage:.2%}",
            "Points For": points_for,
            "Points Against": points_against,
            "Point Differential": point_differential
        }

    except requests.RequestException as e:
        return f"An error occurred: {e}"

# Webcam setup
camera = cv2.VideoCapture(0)

# Valid class names
valid_class_names = ['buf', 'kc', 'pit']

while True:
    ret, image = camera.read()
    image = cv2.resize(image, (224, 224), interpolation=cv2.INTER_AREA)
    cv2.imshow("Webcam Image", image)

    # Image preprocessing
    image = np.asarray(image, dtype=np.float32).reshape(1, 224, 224, 3)
    image = (image / 127.5) - 1

    # Class prediction
    prediction = model.predict(image)
    index = np.argmax(prediction)
    predicted_class_name = class_names[index].strip().lower()
    confidence_score = prediction[0][index]

    # Check for a valid class name
    class_name = next((name for name in valid_class_names if name in predicted_class_name), None)
    
    if class_name and confidence_score > 0.5:
        print("Class:", class_name, "Confidence Score:", confidence_score)
        team_data = get_team_info(class_name)
        if isinstance(team_data, dict):
            for key, value in team_data.items():
                data_str = f"{key}: {value}\n"
                ser.write(data_str.encode('utf-8'))
        else:
            ser.write(str(team_data).encode('utf-8'))

    # Keyboard interrupt to exit
    if cv2.waitKey(1) == 27: # Esc key
        break
        
    time.sleep(10)


camera.release()
cv2.destroyAllWindows()
