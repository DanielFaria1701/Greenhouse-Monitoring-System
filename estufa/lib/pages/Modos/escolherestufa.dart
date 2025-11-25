import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'dart:convert';
import 'package:provider/provider.dart';
import 'package:estufa/providers/valores_provider.dart';

class EscolherIdEstufaPage extends StatefulWidget {
  @override
  _EscolherIdEstufaPageState createState() => _EscolherIdEstufaPageState();
}

class _EscolherIdEstufaPageState extends State<EscolherIdEstufaPage> {
  List<String> estufaIds = [];
  bool isLoading = true;

  @override
  void initState() {
    super.initState();
    _fetchEstufaIds();
  }

  Future<void> _fetchEstufaIds() async {
    final response = await http.get(Uri.parse(
        'https://fcyp8v4zp0.execute-api.eu-west-1.amazonaws.com/default/Projeto_Lambda?TableName=Projeto_DB&getAllIds=1'));

    if (response.statusCode == 200) {
      try {
        final List<dynamic> ids = json.decode(response.body);
        setState(() {
          estufaIds = ids.map((id) => id.toString()).toList();
          isLoading = false;
        });
      } catch (e) {
        setState(() {
          isLoading = false;
        });
        print('Failed to parse estufa IDs: $e');
      }
    } else {
      setState(() {
        isLoading = false;
      });
      print('Failed to load estufa IDs, status code: ${response.statusCode}');
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Escolher Estufa'),
      ),
      body: isLoading
          ? Center(child: CircularProgressIndicator())
          : ListView.builder(
              itemCount: estufaIds.length,
              itemBuilder: (context, index) {
                return Container(
                  margin: EdgeInsets.symmetric(vertical: 4.0, horizontal: 8.0),
                  decoration: BoxDecoration(
                    color: Colors.blueGrey.shade50,
                    borderRadius: BorderRadius.circular(8.0),
                  ),
                  child: ListTile(
                    title: Text(
                      'Estufa Nº: ${estufaIds[index]}',
                      style: TextStyle(fontSize: 18),
                    ),
                    trailing: Icon(Icons.chevron_right),
                    onTap: () {
                      // Set the selected estufa ID in the provider
                      Provider.of<ValoresProvider>(context, listen: false)
                          .setEstufaId(estufaIds[index]);

                      showDialog(
                        context: context,
                        builder: (BuildContext context) {
                          return AlertDialog(
                            title: Text('Confirmação'),
                            content:
                                Text('Você escolheu o ID: ${estufaIds[index]}'),
                            actions: <Widget>[
                              TextButton(
                                onPressed: () {
                                  Navigator.of(context).pop();
                                },
                                child: Text('OK'),
                              ),
                            ],
                          );
                        },
                      );
                    },
                  ),
                );
              },
            ),
    );
  }
}
