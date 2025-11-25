# Estufa + - Greenhouse Monitoring System
The Greenhouse Monitoring System is an IoT solution developed to monitor and control environmental conditions in agricultural greenhouses. The system collects temperature, humidity, and light data through sensors connected to a Zolertia module, sending them to the AWS cloud where they are processed and made available to the user in real time.

The architecture integrates MQTT communication with TLS/SSL, data storage and processing on AWS DynamoDB, and a mobile application developed in Flutter. The app displays real-time information, historical statistics, and allows users to set target values or select predefined profiles for different crops.

The project was built with a focus on scalability and ease of use, making it easy to expand with new sensors or additional greenhouses.
