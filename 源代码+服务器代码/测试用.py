import requests

url = "http://127.0.0.1:5000/upload"  # 本机测试用
data = "TEMP:27.35\r\n"

response = requests.post(url, data=data)
print(response.text)