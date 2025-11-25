import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:estufa/providers/valores_provider.dart';
import 'package:http/http.dart' as http;
import 'dart:convert';
import 'dart:async';

class InfoTable extends StatefulWidget {
  @override
  _InfoTableState createState() => _InfoTableState();
}

class _InfoTableState extends State<InfoTable> {
  Map<String, dynamic>? lastReadValues;
  bool isLoading = false;
  late Timer timer;

  @override
  void initState() {
    super.initState();
    fetchData();
    timer = Timer.periodic(Duration(seconds: 15), (Timer t) => fetchData());
  }

  @override
  void dispose() {
    timer.cancel();
    super.dispose();
  }

  Future<void> fetchData() async {
    setState(() {
      isLoading = true;
    });

    try {
      final estufaId =
          Provider.of<ValoresProvider>(context, listen: false).estufaId;
      final url = Uri.parse(
          'https://fcyp8v4zp0.execute-api.eu-west-1.amazonaws.com/default/Projeto_Lambda');
      final response = await http.get(
        url.replace(queryParameters: {
          'TableName': 'Projeto_DB',
          'action': 'latest',
          'id': estufaId,
          'limit': '1'
        }),
      );

      print('Request URL: $url');
      print('Response status: ${response.statusCode}');
      print('Response body: ${response.body}');

      if (response.statusCode == 200) {
        setState(() {
          List<dynamic> jsonResponse = jsonDecode(response.body);

          if (jsonResponse.isNotEmpty) {
            lastReadValues = jsonResponse[0];
            compareAndCallLambda();
          } else {
            throw Exception('No items found in the response');
          }
        });
      } else {
        throw Exception('Failed to load last read values');
      }
    } catch (error) {
      print('Error fetching data: $error');
    } finally {
      setState(() {
        isLoading = false;
      });
    }
  }

  void compareAndCallLambda() {
    try {
      final valoresProvider =
          Provider.of<ValoresProvider>(context, listen: false);
      String? realTemperatureStr = lastReadValues?['Temperature']?.toString();
      String? theoreticalTemperatureStr =
          valoresProvider.valores['temperatura'];
      String? realHumidityStr = lastReadValues?['Humidity']?.toString();
      String? theoreticalHumidityStr = valoresProvider.valores['humidade'];
      String? realLightStr = lastReadValues?['Light']?.toString();
      String? theoreticalLightStr = valoresProvider.valores['luz'];
      String lambdaPayload =
          '{"action": "sendDeviceStates", "ID": "${valoresProvider.estufaId}",';

      if (realTemperatureStr != null && theoreticalTemperatureStr != 'N') {
        double realTemperature = double.parse(realTemperatureStr);
        double theoreticalTemperature =
            double.parse(theoreticalTemperatureStr!);
        double differenceTemp = realTemperature - theoreticalTemperature;

        if (differenceTemp < -1) {
          lambdaPayload += '"Aquecedor": "1", "Ventilador": "0",';
        } else if (differenceTemp > 1) {
          lambdaPayload += '"Aquecedor": "0", "Ventilador": "1",';
        } else {
          lambdaPayload += '"Aquecedor": "0", "Ventilador": "0",';
        }
      } else {
        lambdaPayload += '"Aquecedor": "0", "Ventilador": "0",';
      }

      if (realHumidityStr != null && theoreticalHumidityStr != 'N') {
        double realHumidity = double.parse(realHumidityStr);
        double theoreticalHumidity = double.parse(theoreticalHumidityStr!);
        double differenceHum = realHumidity - theoreticalHumidity;

        if (differenceHum < -1) {
          lambdaPayload += ' "Humidificador": "1", "Desumidificador": "0",';
        } else if (differenceHum > 1) {
          lambdaPayload += ' "Humidificador": "0", "Desumidificador": "1",';
        } else {
          lambdaPayload += ' "Humidificador": "0", "Desumidificador": "0",';
        }
      } else {
        lambdaPayload += ' "Humidificador": "0", "Desumidificador": "0",';
      }

      if (realLightStr != null && theoreticalLightStr != 'N') {
        double realLight = double.parse(realLightStr);
        double theoreticalLight = double.parse(theoreticalLightStr!);
        double differenceLight = realLight - theoreticalLight;

        if (differenceLight < -100) {
          lambdaPayload += ' "Tapar_Luz": "0", "Dar_Luz": "1"}';
        } else if (differenceLight > 100) {
          lambdaPayload += ' "Tapar_Luz": "1", "Dar_Luz": "0"}';
        } else {
          lambdaPayload += ' "Tapar_Luz": "0", "Dar_Luz": "0"}';
        }
      } else {
        lambdaPayload += ' "Tapar_Luz": "0", "Dar_Luz": "0"}';
      }
      callLambdaRule(lambdaPayload);
    } catch (error) {
      print('Error comparing and calling Lambda: $error');
    }
  }

  Future<void> callLambdaRule(String body) async {
    final url = Uri.parse(
        'https://fcyp8v4zp0.execute-api.eu-west-1.amazonaws.com/default/Projeto_Lambda');
    final response = await http.post(
      url,
      body: body,
    );

    print('Called Lambda with: $body');
    print('Lambda Response status: ${response.statusCode}');
    print('Lambda Response body: ${response.body}');
  }

  Color getColor(String? realValueStr, String? theoreticalValueStr) {
    try {
      if (realValueStr != null && theoreticalValueStr != null) {
        double realValue = double.parse(realValueStr);
        double theoreticalValue = double.parse(theoreticalValueStr);

        double difference = realValue - theoreticalValue;

        if (difference > 1) {
          return Colors.pink;
        } else if (difference < -1) {
          return Colors.blue;
        }
      }
    } catch (e) {
      print('Error parsing double: $e');
    }

    return Colors.black;
  }

  Color getColor2(String? realValueStr, String? theoreticalValueStr) {
    try {
      if (realValueStr != null && theoreticalValueStr != null) {
        double realValue = double.parse(realValueStr);
        double theoreticalValue = double.parse(theoreticalValueStr);

        double difference = realValue - theoreticalValue;

        if (difference > 100) {
          return Colors.pink;
        } else if (difference < -100) {
          return Colors.blue;
        }
      }
    } catch (e) {
      print('Error parsing double: $e');
    }

    return Colors.black;
  }

  Color getControlledColor(
      Map<String, dynamic>? lastReadValues, ValoresProvider valoresProvider) {
    bool isTemperatureControlled = isParameterControlled1(
      lastReadValues?['Temperature']?.toString(),
      valoresProvider.valores['temperatura'],
    );
    bool isHumidityControlled = isParameterControlled1(
      lastReadValues?['Humidity']?.toString(),
      valoresProvider.valores['humidade'],
    );
    bool isLightControlled = isParameterControlled2(
      lastReadValues?['Light']?.toString(),
      valoresProvider.valores['luz'],
    );

    int uncontrolledCount = 0;
    if (!isTemperatureControlled) uncontrolledCount++;
    if (!isHumidityControlled) uncontrolledCount++;
    if (!isLightControlled) uncontrolledCount++;

    if ((isTemperatureControlled &&
            isHumidityControlled &&
            isLightControlled) ||
        (valoresProvider.valores['temperatura'] == 'N' &&
            valoresProvider.valores['humidade'] == 'N' &&
            valoresProvider.valores['luz'] == 'N')) {
      return Colors.green;
    } else if (uncontrolledCount == 1) {
      return Colors.yellow;
    } else if (uncontrolledCount == 2) {
      return Colors.orange;
    } else {
      return Colors.red;
    }
  }

  bool isParameterControlled1(
      String? realValueStr, String? theoreticalValueStr) {
    try {
      if (realValueStr != null && theoreticalValueStr != null) {
        double realValue = double.parse(realValueStr);
        double theoreticalValue = double.parse(theoreticalValueStr);

        double difference = (realValue - theoreticalValue).abs();

        if (difference <= 1) {
          return true; // Parâmetro controlado
        }
      }
    } catch (e) {
      print('Error parsing double: $e');
    }
    return false;
  }

  bool isParameterControlled2(
      String? realValueStr, String? theoreticalValueStr) {
    try {
      if (realValueStr != null && theoreticalValueStr != null) {
        double realValue = double.parse(realValueStr);
        double theoreticalValue = double.parse(theoreticalValueStr);

        double difference = (realValue - theoreticalValue).abs();

        if (difference <= 100) {
          return true; // Parâmetro controlado
        }
      }
    } catch (e) {
      print('Error parsing double: $e');
    }
    return false;
  }

  @override
  Widget build(BuildContext context) {
    final valoresProvider = Provider.of<ValoresProvider>(context);

    if (valoresProvider.estufaId.isEmpty) {
      return Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Text(
              'Informações',
              style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
            ),
            Text(
              'Nenhuma Estufa Selecionada',
              style: TextStyle(fontSize: 18),
            ),
          ],
        ),
      );
    }
    return Column(
      children: [
        Padding(
          padding: const EdgeInsets.all(8.0),
          child: Text(
            'Informações - Estufa Nº ${valoresProvider.estufaId}',
            style: TextStyle(
              fontSize: 20,
              fontWeight: FontWeight.bold,
            ),
          ),
        ),
        Padding(
          padding: const EdgeInsets.all(8.0),
          child: Text(
            'CONTROLADO',
            style: TextStyle(
              fontSize: 20,
              fontWeight: FontWeight.bold,
              color: getControlledColor(lastReadValues, valoresProvider),
            ),
          ),
        ),
        isLoading
            ? Center(child: CircularProgressIndicator())
            : Table(
                columnWidths: const {
                  0: FlexColumnWidth(1),
                  1: FlexColumnWidth(1),
                  2: FlexColumnWidth(1),
                },
                border: TableBorder.all(color: Colors.transparent),
                children: [
                  const TableRow(
                    children: [
                      Padding(
                        padding: EdgeInsets.all(8.0),
                        child: Text(
                          'Parâmetro',
                          style: TextStyle(
                              fontSize: 16, fontWeight: FontWeight.bold),
                        ),
                      ),
                      Padding(
                        padding: EdgeInsets.all(8.0),
                        child: Text(
                          'Real',
                          style: TextStyle(
                              fontSize: 16, fontWeight: FontWeight.bold),
                        ),
                      ),
                      Padding(
                        padding: EdgeInsets.all(8.0),
                        child: Text(
                          'Teórico',
                          style: TextStyle(
                              fontSize: 16, fontWeight: FontWeight.bold),
                        ),
                      ),
                    ],
                  ),
                  TableRow(
                    children: [
                      const Padding(
                        padding: EdgeInsets.symmetric(vertical: 16.0),
                        child: Text('Temperatura:',
                            style: TextStyle(fontSize: 16)),
                      ),
                      Padding(
                        padding: const EdgeInsets.symmetric(vertical: 16.0),
                        child: Text(
                          lastReadValues?['Temperature']?.toString() ?? '',
                          style: TextStyle(
                            fontSize: 16,
                            color: getColor(
                              lastReadValues?['Temperature']?.toString(),
                              valoresProvider.valores['temperatura'],
                            ),
                          ),
                        ),
                      ),
                      Padding(
                        padding: const EdgeInsets.symmetric(vertical: 16.0),
                        child: Text(
                          valoresProvider.valores['temperatura'] ?? '',
                          style: TextStyle(fontSize: 16),
                        ),
                      ),
                    ],
                  ),
                  TableRow(
                    children: [
                      const Padding(
                        padding: EdgeInsets.symmetric(vertical: 16.0),
                        child:
                            Text('Humidade:', style: TextStyle(fontSize: 16)),
                      ),
                      Padding(
                        padding: const EdgeInsets.symmetric(vertical: 16.0),
                        child: Text(
                          lastReadValues?['Humidity']?.toString() ?? '',
                          style: TextStyle(
                            fontSize: 16,
                            color: getColor(
                              lastReadValues?['Humidity']?.toString(),
                              valoresProvider.valores['humidade'],
                            ),
                          ),
                        ),
                      ),
                      Padding(
                        padding: const EdgeInsets.symmetric(vertical: 16.0),
                        child: Text(
                          valoresProvider.valores['humidade'] ?? '',
                          style: TextStyle(fontSize: 16),
                        ),
                      ),
                    ],
                  ),
                  TableRow(
                    children: [
                      const Padding(
                        padding: EdgeInsets.symmetric(vertical: 16.0),
                        child: Text('Luz:', style: TextStyle(fontSize: 16)),
                      ),
                      Padding(
                        padding: const EdgeInsets.symmetric(vertical: 16.0),
                        child: Text(
                          lastReadValues?['Light']?.toString() ?? '',
                          style: TextStyle(
                            fontSize: 16,
                            color: getColor2(
                              lastReadValues?['Light']?.toString(),
                              valoresProvider.valores['luz'],
                            ),
                          ),
                        ),
                      ),
                      Padding(
                        padding: const EdgeInsets.symmetric(vertical: 16.0),
                        child: Text(
                          valoresProvider.valores['luz'] ?? '',
                          style: TextStyle(fontSize: 16),
                        ),
                      ),
                    ],
                  ),
                ],
              ),
      ],
    );
  }
}
