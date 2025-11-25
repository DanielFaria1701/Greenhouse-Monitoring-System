import 'package:flutter/material.dart';
import 'package:estufa/pages/Modos/catalogopage.dart';
import 'package:estufa/pages/Modos/escolhervalorpage.dart';
import 'package:estufa/pages/Modos/escolherestufa.dart';
import 'package:provider/provider.dart';
import 'package:estufa/providers/valores_provider.dart';

class ChooseModePage extends StatelessWidget {
  final String buttonText;

  const ChooseModePage({Key? key, required this.buttonText}) : super(key: key);

  void _handleButtonPress(BuildContext context, String buttonText) {
    print('$buttonText button pressed');

    if (buttonText == "Catálogo predefinido") {
      Navigator.push(
        context,
        MaterialPageRoute(
          builder: (context) => CatalogoPage(),
        ),
      );
    } else if (buttonText == "Escolher valores") {
      Navigator.push(
        context,
        MaterialPageRoute(
          builder: (context) => EscolherValoresPage(),
        ),
      );
    } else if (buttonText == "Não Escolher Valores") {
      // Aqui você pode atualizar os valores para "N"
      Provider.of<ValoresProvider>(context, listen: false)
          .setValores('N', 'N', 'N');
    } else if (buttonText == "Escolher ID da Estufa") {
      Navigator.push(
        context,
        MaterialPageRoute(
          builder: (context) => EscolherIdEstufaPage(),
        ),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white,
      body: Center(
        child: Padding(
          padding: const EdgeInsets.all(20),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              ElevatedButton(
                style: ElevatedButton.styleFrom(
                  backgroundColor: const Color.fromARGB(255, 200, 200, 200),
                ),
                onPressed: () =>
                    _handleButtonPress(context, "Catálogo predefinido"),
                child: Container(
                  width: double.infinity,
                  height: 60,
                  alignment: Alignment.center,
                  child: const Text("Catálogo predefinido",
                      style: TextStyle(fontSize: 18)),
                ),
              ),
              const SizedBox(height: 20),
              ElevatedButton(
                style: ElevatedButton.styleFrom(
                  backgroundColor: const Color.fromARGB(255, 180, 180, 180),
                ),
                onPressed: () =>
                    _handleButtonPress(context, "Escolher valores"),
                child: Container(
                  width: double.infinity,
                  height: 60,
                  alignment: Alignment.center,
                  child: const Text("Escolher valores",
                      style: TextStyle(fontSize: 18)),
                ),
              ),
              const SizedBox(height: 20),
              ElevatedButton(
                style: ElevatedButton.styleFrom(
                  backgroundColor: const Color.fromARGB(255, 160, 160, 160),
                ),
                onPressed: () =>
                    _handleButtonPress(context, "Não Escolher Valores"),
                child: Container(
                  width: double.infinity,
                  height: 60,
                  alignment: Alignment.center,
                  child: const Text("Não Escolher Valores",
                      style: TextStyle(fontSize: 18)),
                ),
              ),
              const SizedBox(height: 20),
              ElevatedButton(
                style: ElevatedButton.styleFrom(
                  backgroundColor: const Color.fromARGB(255, 140, 140, 140),
                ),
                onPressed: () =>
                    _handleButtonPress(context, "Escolher ID da Estufa"),
                child: Container(
                  width: double.infinity,
                  height: 60,
                  alignment: Alignment.center,
                  child: const Text("Escolher Estufa",
                      style: TextStyle(fontSize: 18)),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
