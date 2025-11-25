import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:estufa/providers/valores_provider.dart';
import 'dart:convert';
import 'package:http/http.dart' as http;

class CatalogoPage extends StatelessWidget {
  final TextEditingController temperaturaController = TextEditingController();
  final TextEditingController humidadeController = TextEditingController();
  final TextEditingController luzController = TextEditingController();

  void _submitValues(BuildContext context, String temperatura, String humidade,
      String luz) async {
    print('Temperatura: $temperatura');
    print('Humidade: $humidade');
    print('Luz: $luz');

    String estufaId =
        Provider.of<ValoresProvider>(context, listen: false).estufaId;

    // Atualizar os valores no Provider
    Provider.of<ValoresProvider>(context, listen: false)
        .setValores(temperatura, humidade, luz);

    // Criar o timestamp atual
    int timestamp = DateTime.now().millisecondsSinceEpoch;

    // Criar o corpo da requisição com valores como strings
    final body = {
      "TableName": "Projeto_DB",
      "Item": {
        "ID": estufaId,
        "Timestamp": timestamp,
        "APP": "1",
        "Temperature": temperatura,
        "Humidity": humidade,
        "Light": luz
      }
    };

    // Imprimir o corpo da requisição
    print('Request Body: $body');

    // Enviar a requisição POST
    final response = await http.post(
      Uri.parse(
          'https://fcyp8v4zp0.execute-api.eu-west-1.amazonaws.com/default/Projeto_Lambda'),
      headers: {'Content-Type': 'application/json'},
      body: jsonEncode(body),
    );

    if (response.statusCode == 200) {
      print('Dados enviados com sucesso!');
    } else {
      print('Falha ao enviar os dados: ${response.statusCode}');
      print('Corpo da resposta: ${response.body}');
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Catálogo Predefinido'),
        centerTitle: true,
        backgroundColor: Color.fromARGB(255, 3, 167, 0),
      ),
      body: Padding(
        padding: const EdgeInsets.all(16.0),
        child: GridView.count(
          crossAxisCount: 2,
          crossAxisSpacing: 10,
          mainAxisSpacing: 10,
          children: <Widget>[
            _buildGridItem(
                context, 'strawberry', 'Morango', '18', '75', '8456'),
            _buildGridItem(context, 'lettuce', 'Alface', '10', '80', '2592'),
            _buildGridItem(context, 'fig', 'Figo', '20', '30', '2814'),
            _buildGridItem(context, 'tomato', 'Tomate', '25', '55', '5343'),
          ],
        ),
      ),
    );
  }

  Widget _buildGridItem(BuildContext context, String assetName, String label,
      String temp, String hum, String light) {
    return ElevatedButton(
      style: ElevatedButton.styleFrom(
        backgroundColor: Colors.white,
        foregroundColor: Colors.green,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(8.0),
        ),
      ),
      onPressed: () {
        _showDialog(context, label, temp, hum, light);
      },
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: <Widget>[
          Image.asset('image/$assetName.png', width: 50, height: 50),
          SizedBox(height: 12),
          Text(label),
        ],
      ),
    );
  }

  void _showDialog(BuildContext context, String label, String temp, String hum,
      String light) {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Text('Valores para $label'),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            children: <Widget>[
              Text('Temperatura: $temp°C'),
              Text('Humidade: $hum%'),
              Text('Luz: $light lx'),
            ],
          ),
          actions: <Widget>[
            TextButton(
              child: Text('Cancelar'),
              onPressed: () {
                Navigator.of(context).pop();
              },
            ),
            TextButton(
              child: Text('OK'),
              onPressed: () {
                Provider.of<ValoresProvider>(context, listen: false)
                    .setValores(temp, hum, light);
                Navigator.of(context).pop();
                _submitValues(context, temp, hum,
                    light); // Chamar o envio dos dados com os valores corretos
              },
            ),
          ],
        );
      },
    );
  }
}
