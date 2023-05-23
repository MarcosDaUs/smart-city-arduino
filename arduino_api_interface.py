import requests
import serial

# Establecer la configuración del puerto serial
port = 'COM5'
baudrate = 9600

# URL a la que realizar la solicitud POST
url = 'https://isa.requestcatcher.com'
url_weather = 'https://api.open-meteo.com/v1/forecast?latitude=52.52&longitude=13.41&current_weather=true'

# Inicializar la conexión serial
ser = serial.Serial(port, baudrate)

# Leer los datos serial
while True:
    line = ser.readline().decode().strip()
    if line:
        values = line.split(',')
        sensorValue1 = values[0]
        print(f"Valor del sensor 1: {sensorValue1}")
        # Datos a enviar en la solicitud POST (puede ser un diccionario)
        data = {
            'co2_state': sensorValue1,
        }
        # Realizar la solicitud POST
        response = requests.post(url, data=data)

        # Verificar el estado de la respuesta
        if response.status_code == 200:
            print('Solicitud POST exitosa')
        else:
            print('Error en la solicitud POST')

        if sensorValue1 == "WARNING":
            # Realizar la solicitud GET
            response_get = requests.get(url_weather)

            # Verificar el estado de la respuesta
            if response_get.status_code == 200:
                data = response_get.json()
        
                # Utilizar los datos recibidos
                
                temperature = str(data.get("current_weather").get("temperature")) +'/'
                print(temperature)
                ser.write(temperature.encode()) 
            else:
                print('Error en la solicitud POST')

        
