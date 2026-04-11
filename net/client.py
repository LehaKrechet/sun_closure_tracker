import requests
import json
import time

url = "http://localhost:8080/commands"
#"http://127.0.0.1:8080/"
# The JSON data to send (as a Python dictionary)
data = {
    "commands": "forward",
    "id": 33,
    "time": 1.00
}
def sndpost():
    response = requests.post(url, json=data)
# Check the response
    if response.status_code == 200:
       print("Request successful!")
       print("Response JSON:", response.json())
    else:
       print(f"Request failed with status code {response.status_code}")
       print("Response text:", response.text)
def snd():
    response = requests.get(url, json=data)
# Check the response
    if response.status_code == 200:
       print("Request successful!")
       print(response.json)

    else:
       print(f"Request failed with status code {response.status_code}")
snd()
time.sleep(1) # Pauses the program for 1 second