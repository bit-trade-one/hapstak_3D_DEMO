# hapstak_3D_DEMO

[hapstakデジタル版](https://github.com/bit-trade-one/ADACHACY-hapStak)にて使用可能な  
[ソースコード](https://github.com/bit-trade-one/hapstak_3D_DEMO/blob/master/hapstak_3D_DEMO/hapstak_3D_DEMO.ino) / [3Dプリント用ケース(別リポジトリ)](https://github.com/bit-trade-one/hapstak_3Dprint_Case)を配布しています。

---

![image](https://user-images.githubusercontent.com/85532743/218913817-ff9a2384-bb42-47db-9c99-82d909a57c1d.png)

![image](https://user-images.githubusercontent.com/85532743/218913860-5be94a8b-fd9c-4af2-a71c-0b7d736dbbee.png)

---

# ソースコード書き込み時の注意

1. 39行目の#define FLG_WRITE_FILESを最初１にしてヘッダファイルを読み込み

```cpp
#define FLG_WRITE_FILES 0 // 信号ファイルの書き込みを行うかどうか。信号ファイルを初めて書き込むときや変更したときは1、それ以外は0にする。
```

2.ソースを書き込み後、ファイル書き込み処理のため、M5ATOM Matrixを一度起動する。  
この際起動に1分程度かかります。LED表示が「1」になったら完了。  

3. 39行目の#define FLG_WRITE_FILESを0にして再度ヘッダファイルを読み込み

4. LED表示が「1」になったら完了。  

[詳細情報(動画あり)](https://protopedia.net/prototype/2580?fbclid=IwAR3THhO-MchysjxQ9p1P9tdGA6_OPB8LnAHB-C3bYj59Ag4bQQjA01_XnqY)
