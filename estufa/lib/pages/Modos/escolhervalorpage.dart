import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:estufa/providers/valores_provider.dart';
import 'dart:convert';
import 'package:http/http.dart' as http;

class EscolherValoresPage extends StatelessWidget {
  final TextEditingController temperaturaController = TextEditingController();
  final TextEditingController humidadeController = TextEditingController();
  final TextEditingController luzController = TextEditingController();

  EscolherValoresPage({Key? key}) : super(key: key);

  void _submitValues(BuildContext context) async {
    String temperatura = temperaturaController.text;
    String humidade = humidadeController.text;
    String luz = luzController.text;

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
        "Temperature": "$temperatura",
        "Humidity": "$humidade",
        "Light": "$luz"
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

    // Voltar para a página inicial
    Navigator.pop(context);
  }

  @override
  Widget build(BuildContext context) {
    final valoresProvider = Provider.of<ValoresProvider>(context);
    final valores = valoresProvider.valores;

    return Scaffold(
      appBar: AppBar(
        title: const Text('Escolher Valores'),
      ),
      body: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          children: [
            TextField(
              controller: temperaturaController,
              decoration: InputDecoration(
                labelText: 'Temperatura (Atual: ${valores['temperatura']})',
              ),
              keyboardType: TextInputType.number,
            ),
            TextField(
              controller: humidadeController,
              decoration: InputDecoration(
                labelText: 'Humidade (Atual: ${valores['humidade']})',
              ),
              keyboardType: TextInputType.number,
            ),
            TextField(
              controller: luzController,
              decoration: InputDecoration(
                labelText: 'Luz (Atual: ${valores['luz']})',
              ),
              keyboardType: TextInputType.number,
            ),
            const SizedBox(height: 20),
            ElevatedButton(
              onPressed: () => _submitValues(context),
              child: const Text('Submit'),
            ),
          ],
        ),
      ),
    );
  }
}
