import requests
from keras.models import load_model
from PIL import Image, ImageOps
import numpy as np

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

# Disable scientific notation for clarity
np.set_printoptions(suppress=True)

# Load the model and labels
model = load_model("keras_Model.h5", compile=False)
class_names = [line.strip() for line in open("labels.txt", "r").readlines()]

# Prepare the image
data = np.ndarray(shape=(1, 224, 224, 3), dtype=np.float32)
image = Image.open("buf.png").convert("RGB")
size = (224, 224)
image = ImageOps.fit(image, size, Image.Resampling.LANCZOS)
image_array = np.asarray(image)
normalized_image_array = (image_array.astype(np.float32) / 127.5) - 1
data[0] = normalized_image_array

# Predict the class
prediction = model.predict(data)
index = np.argmax(prediction)
predicted_class_name = class_names[index].strip().lower()

# Validate class name
valid_class_names = ['buf', 'kc', 'pit']
class_name = None
for valid_name in valid_class_names:
    if valid_name in predicted_class_name:
        class_name = valid_name
        break

# Proceed only if a valid class name is found
if class_name:
    confidence_score = prediction[0][index]
    print("Predicted Class Name:", class_name)
    print("Confidence Score:", confidence_score)

    # Fetch and display team info if confidence is high enough
    if confidence_score > 0.5:
        team_data = get_team_info(class_name)
        if isinstance(team_data, dict):
            for key, value in team_data.items():
                print(f"{key}: {value}")
        else:
            print(team_data)
else:
    print("No valid team class name found in prediction.")
