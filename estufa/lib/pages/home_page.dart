import 'package:flutter/material.dart';
import 'infopage.dart';
import 'statisticspage.dart';
import 'choosemodepage.dart';
import 'mainpage.dart';

class HomePage extends StatefulWidget {
  @override
  _HomePageState createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  int _selectedIndex = 0;

  final List<Widget> _pages = [
    MainPage(),
    InfoPage(),
    StatisticsPage(animate: true),
    ChooseModePage(buttonText: "Default Text"),
  ];

  void _onItemTapped(int index) {
    setState(() {
      _selectedIndex = index;
    });
  }

  @override
  Widget build(BuildContext context) {
    const String appTitle = 'Estufa+';
    return Scaffold(
      appBar: AppBar(
        title: const Text(appTitle),
        centerTitle: true, // Center the title
        backgroundColor: Color.fromARGB(255, 3, 167, 0),
      ),
      body: _pages[_selectedIndex],
      bottomNavigationBar: BottomNavigationBar(
        items: const <BottomNavigationBarItem>[
          BottomNavigationBarItem(
            icon: Icon(Icons.home),
            label: 'Home',
          ),
          BottomNavigationBarItem(
            icon: Icon(Icons.info),
            label: 'Info Atual',
          ),
          BottomNavigationBarItem(
            icon: Icon(Icons.bar_chart),
            label: 'Estatísticas',
          ),
          BottomNavigationBarItem(
            icon: Icon(Icons.settings),
            label: 'Escolher Modo',
          ),
        ],
        currentIndex: _selectedIndex,
        selectedItemColor: Colors.green, // Color for the selected item
        unselectedItemColor: Colors.black, // Color for the unselected items
        onTap: _onItemTapped,
      ),
    );
  }
}
