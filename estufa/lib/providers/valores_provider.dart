import 'package:flutter/material.dart';

class ValoresProvider with ChangeNotifier {
  Map<String, Map<String, String>> _valores = {};
  String _estufaId = '';

  String get estufaId => _estufaId;

  Map<String, String> get valores =>
      _valores[_estufaId] ?? {'temperatura': '', 'humidade': '', 'luz': ''};

  void setValores(String temperatura, String humidade, String luz) {
    if (_estufaId.isNotEmpty) {
      _valores[_estufaId] = {
        'temperatura': temperatura,
        'humidade': humidade,
        'luz': luz
      };
      notifyListeners();
    }
  }

  void setEstufaId(String estufaId) {
    _estufaId = estufaId;
    notifyListeners();
  }
}
