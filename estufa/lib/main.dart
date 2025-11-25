import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'providers/valores_provider.dart';
import 'pages/home_page.dart';

void main() {
  runApp(
    ChangeNotifierProvider(
      create: (context) => ValoresProvider(),
      child: MyApp(),
    ),
  );
}

class MyApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Estufa+',
      theme: ThemeData(
        primarySwatch: Colors.blue,
      ),
      home: HomePage(),
    );
  }
}
