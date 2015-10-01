# OV528 samples

GroveのシリアルカメラキットとESP-WROOM-02(WiFiモジュール)を組み合わせて色々する雑多なもの．

- Grove-シリアルカメラキット http://akizukidenshi.com/catalog/g/gM-09161/
- ESP-WROOM-02 http://akizukidenshi.com/catalog/g/gM-09607/


## arduino/HelloServer

ESP-WROOM-02と組み合わせてWebブラウザで写真を表示．

Arduino IDE に https://github.com/esp8266/Arduino をインストールしてESP-WROOM-02に書き込んでください．

Arduinoと言いつつ，Arduinoは不要です．


### avr

AVRでOV528からJpegでのデータの読み出しするサンプル．カメラの動作確認用に書いたサンプル．

RX,TXだけ接続すれば動きます．20MHzのクロック推奨．8MHzの内部オシレータではタイミングが合わないのでシリアルの通信速度を下げてください．


## TODO

- コード整理
- 写真のアップロード先のサーバ．


## License

- MIT License

