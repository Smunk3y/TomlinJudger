import requests

def get_team_info(url):
    try:
        response = requests.get(url)
        response.raise_for_status()
        data = response.json()

        # Extracting team location
        team_location = data.get("team", {}).get("location", "No location found")

        # Extracting team record and win percentage
        team_record_items = data.get("team", {}).get("record", {}).get("items", [])
        if team_record_items:
            total_record = team_record_items[0]
            team_record = total_record.get("summary", "No record found")
            win_percentage = total_record.get("stats", [{}])[17].get("value", 0)  # winPercent
        else:
            team_record = "No record items"
            win_percentage = 0

        # Extracting points for, points against, and point differential
        points_for = next((stat.get("value", 0) for stat in total_record.get("stats", []) if stat.get("name") == "pointsFor"), 0)
        points_against = next((stat.get("value", 0) for stat in total_record.get("stats", []) if stat.get("name") == "pointsAgainst"), 0)
        point_differential = next((stat.get("value", 0) for stat in total_record.get("stats", []) if stat.get("name") == "pointDifferential"), 0)

        return team_location, team_record, win_percentage, points_for, points_against, point_differential

    except requests.RequestException as e:
        return f"An error occurred: {e}"

# URL for the Pittsburgh Steelers team
url = "https://site.api.espn.com/apis/site/v2/sports/football/nfl/teams/pit"

# Call the function and print the results
team_location, team_record, win_percentage, points_for, points_against, point_differential = get_team_info(url)
print("Team Location:", team_location)
print("Team Record:", team_record)
print("Win Percentage:", f"{win_percentage:.2%}")
print("Points For:", points_for)
print("Points Against:", points_against)
print("Point Differential:", point_differential)
