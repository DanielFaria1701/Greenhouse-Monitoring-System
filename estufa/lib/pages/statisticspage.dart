import 'package:flutter/material.dart';
import 'package:charts_flutter/flutter.dart' as charts;
import 'dart:convert';
import 'package:http/http.dart' as http;
import 'package:provider/provider.dart';
import 'package:estufa/providers/valores_provider.dart';
import 'average_data.dart';
import 'dart:async';

class StatisticsPage extends StatefulWidget {
  final bool animate;

  StatisticsPage({required this.animate});

  @override
  _StatisticsPageState createState() => _StatisticsPageState();
}

class _StatisticsPageState extends State<StatisticsPage> {
  late Future<List<charts.Series<AverageData, DateTime>>> _seriesList;
  late Future<Map<String, String>> _averages;
  late Timer timer;

  @override
  void initState() {
    super.initState();
    _fetchDataAndAverages();
    // Iniciar o timer para refresh a cada 15 segundos
    timer = Timer.periodic(Duration(seconds: 15), (timer) {
      _refreshData();
    });
  }

  @override
  void dispose() {
    timer.cancel(); // Cancelar o timer ao sair da página
    super.dispose();
  }

  void _refreshData() {
    _fetchDataAndAverages();
  }

  void _fetchDataAndAverages() {
    setState(() {
      _seriesList = _fetchData();
      _averages = _fetchAverages();
    });
  }

  Future<List<charts.Series<AverageData, DateTime>>> _fetchData() async {
    final estufaId =
        Provider.of<ValoresProvider>(context, listen: false).estufaId;

    final temperatureData = await _fetchAttributeData('Temperature', estufaId);
    final humidityData = await _fetchAttributeData('Humidity', estufaId);
    final lightData = await _fetchAttributeData('Light', estufaId);

    return [
      charts.Series<AverageData, DateTime>(
        id: 'Temperatura',
        colorFn: (_, __) => charts.MaterialPalette.blue.shadeDefault,
        domainFn: (AverageData sales, _) => sales.time,
        measureFn: (AverageData sales, _) => sales.value,
        data: temperatureData,
      ),
      charts.Series<AverageData, DateTime>(
        id: 'Humidade',
        colorFn: (_, __) => charts.MaterialPalette.green.shadeDefault,
        domainFn: (AverageData sales, _) => sales.time,
        measureFn: (AverageData sales, _) => sales.value,
        data: humidityData,
      ),
      charts.Series<AverageData, DateTime>(
        id: 'Luz',
        colorFn: (_, __) => charts.MaterialPalette.red.shadeDefault,
        domainFn: (AverageData sales, _) => sales.time,
        measureFn: (AverageData sales, _) => sales.value,
        data: lightData,
      ),
    ];
  }

  Future<List<AverageData>> _fetchAttributeData(
      String attribute, String estufaId) async {
    final response = await http.get(Uri.parse(
        'https://fcyp8v4zp0.execute-api.eu-west-1.amazonaws.com/default/Projeto_Lambda?TableName=Projeto_DB&attribute=$attribute&id=$estufaId'));

    if (response.statusCode == 200) {
      final Map<String, dynamic> responseData = json.decode(response.body);

      if (responseData.containsKey('Items')) {
        final List<dynamic> items = responseData['Items'];

        List<AverageData> data = items.map((item) {
          final timestamp = item['Timestamp'];
          final value = item[attribute] as String?;

          if (timestamp != null && value != null) {
            double parsedValue = double.parse(value);

            // Adjust value based on attribute (e.g., divide "Luz" by 100)
            if (attribute == 'Light') {
              parsedValue /= 100;
            }

            return AverageData(
              time: DateTime.fromMillisecondsSinceEpoch(timestamp),
              value: parsedValue,
            );
          } else {
            throw Exception('Null value found for $attribute');
          }
        }).toList();

        // Ordenar os dados pela data
        data.sort((a, b) => a.time.compareTo(b.time));

        return data;
      } else {
        throw Exception('Items not found in response');
      }
    } else {
      throw Exception('Failed to load $attribute data: ${response.statusCode}');
    }
  }

  Future<Map<String, String>> _fetchAverages() async {
    final estufaId =
        Provider.of<ValoresProvider>(context, listen: false).estufaId;

    final response = await http.get(Uri.parse(
        'https://fcyp8v4zp0.execute-api.eu-west-1.amazonaws.com/default/Projeto_Lambda?TableName=Projeto_DB&average=1&id=$estufaId'));

    if (response.statusCode == 200) {
      final Map<String, dynamic> responseData = json.decode(response.body);

      // Obter os valores e ajustar a luz dividindo por 100
      double averageLight =
          double.parse(responseData['averageLight'].toString()) / 100;

      final Map<String, String> averages = {
        'averageTemperature': responseData['averageTemperature'].toString(),
        'averageHumidity': responseData['averageHumidity'].toString(),
        'averageLight': averageLight.toStringAsFixed(2),
      };

      return averages;
    } else {
      throw Exception('Failed to load averages: ${response.statusCode}');
    }
  }

  @override
  Widget build(BuildContext context) {
    final estufaId = Provider.of<ValoresProvider>(context).estufaId;

    if (estufaId.isEmpty) {
      return Scaffold(
        appBar: AppBar(
          title: Text('Estatísticas'),
        ),
        body: Center(
          child: Text('Nenhuma Estufa Selecionada',
              style: TextStyle(fontSize: 18)),
        ),
      );
    }
    return Scaffold(
      appBar: AppBar(
        title: Text('Estatísticas - Estufa Nº $estufaId'),
      ),
      body: Center(
        child: FutureBuilder<List<charts.Series<AverageData, DateTime>>>(
          future: _seriesList,
          builder: (context, snapshot) {
            if (snapshot.connectionState == ConnectionState.waiting) {
              return CircularProgressIndicator();
            } else if (snapshot.hasError) {
              return Text('Error: ${snapshot.error}');
            } else if (!snapshot.hasData || snapshot.data!.isEmpty) {
              return Text('No data available');
            } else {
              return Column(
                children: [
                  Padding(
                    padding: const EdgeInsets.all(8.0),
                    child: Column(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        Text(
                          'Valores ao Longo do Tempo',
                          style: Theme.of(context).textTheme.headlineSmall,
                        ),
                        SizedBox(height: 12.0), // Espaço entre os textos
                        Text(
                          '(Últimas 24h)',
                          style: TextStyle(
                            fontSize: 16.0,
                            color: Colors.black,
                          ),
                        ),
                      ],
                    ),
                  ),
                  SizedBox(
                    height: 250.0, // Reduzindo a altura do gráfico
                    child: charts.TimeSeriesChart(
                      snapshot.data!,
                      animate: widget.animate,
                      dateTimeFactory: const charts.LocalDateTimeFactory(),
                      behaviors: [
                        charts.SeriesLegend(
                          position: charts.BehaviorPosition.bottom,
                          outsideJustification:
                              charts.OutsideJustification.middleDrawArea,
                          horizontalFirst: false,
                          cellPadding: EdgeInsets.only(right: 4.0, bottom: 4.0),
                          showMeasures: true,
                          measureFormatter: (num? value) {
                            return value == null ? '-' : '$value';
                          },
                        ),
                      ],
                    ),
                  ),
                  SizedBox(height: 50),
                  FutureBuilder<Map<String, String>>(
                    future: _averages,
                    builder: (context, snapshot) {
                      if (snapshot.connectionState == ConnectionState.waiting) {
                        return CircularProgressIndicator();
                      } else if (snapshot.hasError) {
                        return Text('Error: ${snapshot.error}');
                      } else if (!snapshot.hasData) {
                        return Text('No averages available');
                      } else {
                        return Column(
                          children: [
                            Text(
                              'Médias:',
                              style: Theme.of(context)
                                  .textTheme
                                  .headlineSmall!
                                  .copyWith(color: Colors.black87),
                            ),
                            SizedBox(height: 12),
                            Text(
                              'Temperatura: ${snapshot.data!['averageTemperature']}',
                              style:
                                  TextStyle(fontSize: 18, color: Colors.blue),
                            ),
                            Text(
                              'Humidade: ${snapshot.data!['averageHumidity']}',
                              style:
                                  TextStyle(fontSize: 18, color: Colors.green),
                            ),
                            Text(
                              'Luz: ${snapshot.data!['averageLight']}',
                              style: TextStyle(fontSize: 18, color: Colors.red),
                            ),
                          ],
                        );
                      }
                    },
                  ),
                ],
              );
            }
          },
        ),
      ),
    );
  }
}
