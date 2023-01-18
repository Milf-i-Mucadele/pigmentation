import 'dart:io';

import 'package:flutter/material.dart';
import 'package:image_picker/image_picker.dart';
import 'package:open_cv_example/ffi.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:image/image.dart' as img_lib;

void main() {
  runApp(MyApp());
}

class MyApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'Neyasis Teknolojileri FFI Örneği',
      theme: ThemeData(primarySwatch: Colors.red),
      home: Scaffold(
        appBar: AppBar(title: Text('Shabby Dev')),
        body: SafeArea(child: MyHomePage()),
      ),
    );
  }
}

class MyHomePage extends StatefulWidget {
  @override
  _MyHomePageState createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  String? imagePath;
  int processMillisecond = 0;

  @override
  void initState() {
    print("OpenCV Version: ${getOpenCVVersion()}");
    Permission.manageExternalStorage
        .request()
        .then((value) => print("manageExternalStorage: $value"));
    Permission.storage.request().then((value) => print("storage: $value"));
    super.initState();
  }

  void _onSelectImageClick() async {
    final image = await ImagePicker()
        .pickImage(source: ImageSource.gallery, imageQuality: 100);
    if (image == null) return;

    setState(() => this.imagePath = image.path);

    img_lib.Image? image_resized =
        img_lib.decodeImage(new File(this.imagePath!).readAsBytesSync());

    // Resize the image to a 120x? thumbnail (maintaining the aspect ratio).
    img_lib.Image thumbnail = img_lib.copyResize(image_resized!, width: 600);

    // Save the thumbnail as a PNG.
    new File(this.imagePath!)..writeAsBytesSync(img_lib.encodePng(thumbnail));
  }

  void _onConvertClick() async {
    if (imagePath != null) {
      List<String> outputPath = imagePath!.split(".");
      outputPath[outputPath.length - 2] =
          "${outputPath[outputPath.length - 2]}_gray";
      print(outputPath.join("."));
      Stopwatch stopwatch = new Stopwatch()..start();
      convertImageToGrayImage(imagePath!, outputPath.join("."));
      print('Image convert executed in ${stopwatch.elapsed}');
      processMillisecond = stopwatch.elapsedMilliseconds;
      stopwatch.stop();
      this.setState(() {
        imagePath = outputPath.join(".");
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: <Widget>[
          Expanded(
              child: imagePath != null
                  ? Image.file(File(imagePath!), gaplessPlayback: true)
                  : Container()),
          Text(processMillisecond > 0
              ? "Process Millisecond: $processMillisecond"
              : "-"),
          MaterialButton(
            color: Colors.white,
            onPressed: _onSelectImageClick,
            child: Text("Select Image", style: TextStyle(color: Colors.black)),
          ),
          MaterialButton(
            color: Colors.black,
            onPressed: _onConvertClick,
            child: Text("Convert Gray Image",
                style: TextStyle(color: Colors.white)),
          ),
        ],
      ),
    );
  }
}
