import 'package:flutter/material.dart';

class MainPage extends StatelessWidget {
  const MainPage({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        Flexible(
          child: Image.asset(
            'image/playstore.png',
            fit: BoxFit.contain,
            width: double.infinity,
          ),
        ),
      ],
    );
  }
}
