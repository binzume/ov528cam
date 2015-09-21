# OV528 test

AVRで下記カメラからJpegでのデータの読み出しするサンプル．

- Grove-シリアルカメラキット http://akizukidenshi.com/catalog/g/gM-09161/
- ESP-WROOM-02 http://akizukidenshi.com/catalog/g/gM-09607/


## avr

RX,TXだけ接続すれば動きます．20MHzのクロック推奨．8MHzの内部オシレータではタイミングが合わないのでシリアルの通信速度を下げてください．


## Arduino

ESP-WROOM-02と組み合わせてWebブラウザで写真を表示．

Arduinoと言いつつ，Arduino不要です．


## License:

- MIT License

