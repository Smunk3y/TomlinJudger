import requests
from keras.models import load_model
import cv2
import numpy as np
import time

# Team information fetching function
def get_team_info(class_name):
    base_url = "https://site.api.espn.com/apis/site/v2/sports/football/nfl/teams/"
    url = base_url + class_name.strip()  # Removing any leading/trailing whitespace

    try:
        response = requests.get(url)
        response.raise_for_status()
        data = response.json()
        # Similar extraction process as before
        # ...
        return team_data  # Replace with actual data extraction logic

    except requests.RequestException as e:
        return f"An error occurred: {e}"

# Load the model and labels
model = load_model("keras_Model.h5", compile=False)
class_names = open("labels.txt", "r").readlines()

# Set up the camera
camera = cv2.VideoCapture(0)

while True:
    ret, image = camera.read()
    image = cv2.resize(image, (224, 224), interpolation=cv2.INTER_AREA)
    cv2.imshow("Webcam Image", image)

    image = np.asarray(image, dtype=np.float32).reshape(1, 224, 224, 3)
    image = (image / 127.5) - 1

    prediction = model.predict(image)
    index = np.argmax(prediction)
    class_name = class_names[index].strip()  # Class name with whitespace removed
    confidence_score = prediction[0][index]

    print("Class:", class_name[2:], end="")
    print("Confidence Score:", str(np.round(confidence_score * 100))[:-2], "%")

    # Fetch and display team info if confidence is high enough
    if confidence_score > 0.5:  # Adjust this threshold as needed
        team_data = get_team_info(class_name[2:])
        print(team_data)  # Display the fetched team data

    if cv2.waitKey(1) == 27:  # Exit on 'ESC'
        break

    time.sleep(1)

camera.release()
cv2.destroyAllWindows()
