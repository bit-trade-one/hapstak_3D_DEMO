# hapstak_3D_DEMO

[hapstakデジタル版](https://github.com/bit-trade-one/ADACHACY-hapStak)にて使用可能な発展的なデモを配布しています。  
[ソースコード](https://github.com/bit-trade-one/hapstak_3D_DEMO/blob/master/hapstak_3D_DEMO/hapstak_3D_DEMO.ino) / [3Dプリント用ケース](https://github.com/bit-trade-one/ADACHACY-hapStak/tree/master/3Dprint_Case)

# ソースコード書き込み時の注意

1. 39行目の#define FLG_WRITE_FILESを初回は１にしてソースを書き込む。    

該当箇所
```cpp
#define FLG_WRITE_FILES 0 // 信号ファイルの書き込みを行うかどうか。信号ファイルを初めて書き込むときや変更したときは1、それ以外は0にする。
```

2.ファイル書き込み処理のため、M5ATOM Matrixを一度起動する。  
この際起動に1分程度かかります。LED表示が「1」になったら完了。  

3. 39行目の#define FLG_WRITE_FILESを0にして再度ソースを書き込む。    

4. LED表示が「1」になったら完了。  

[詳細情報(参考動画あり)](https://protopedia.net/prototype/2580?fbclid=IwAR3THhO-MchysjxQ9p1P9tdGA6_OPB8LnAHB-C3bYj59Ag4bQQjA01_XnqY)

# 本デモで体験できる振動一覧

| 表示 | 振動名                    | 
|------|--------------------------|
| 1    | Ultra low Sine (33Hz)    | 
| 2    | Super Low Sine (50Hz)    |
| 3    | Low Sine (60Hz)          | 
| 4    | Middle Sine (160Hz)      | 
| 5    | Up Sweep                 |
| 6    | Down Sweep               | 
| 7    | Up Down Sweep            |
| 8    | Mix Wave 1 (Slow)        | 
| 9    | Mix Wave 2 (Fast)        | 
| A    | Heart Beat               | 
| B    | Forward/Backward 1 (Slow)| 
| C    | Forward/Backward 2 (Fast)| 


# 本体構成参考

![image](https://user-images.githubusercontent.com/85532743/218913817-ff9a2384-bb42-47db-9c99-82d909a57c1d.png)

![image](https://user-images.githubusercontent.com/85532743/218913860-5be94a8b-fd9c-4af2-a71c-0b7d736dbbee.png)

---

