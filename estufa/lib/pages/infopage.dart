import 'package:flutter/material.dart';
import '../widgets/table_widget.dart';

class InfoPage extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          children: [
            Spacer(flex: 2), // Espaço acima com flex menor
            Center(
              child: InfoTable(),
            ),
            Spacer(flex: 3), // Espaço abaixo com flex maior
          ],
        ),
      ),
    );
  }
}
