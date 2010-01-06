echo off

if not exist bunruidb.txt goto error

perl bgh.pl < bunruidb.txt > bgh.dat
make_db bgh.db < bgh.dat
del bgh.dat
make_db sm2code.db < sm2code.dat

echo 分類語彙表データベースが作成されました。
goto end

:error
echo 分類語彙表データベースの作成に失敗しました。
goto end

:end
