echo off

if not exist bunruidb.txt goto error

perl bgh.pl < bunruidb.txt > bgh.dat
del /F bgh.db
make_db bgh.db < bgh.dat
del /F sm2code.db
make_db sm2code.db < sm2code.dat

echo 分類語彙表データベースが作成されました。
goto end

:error
echo 分類語彙表データベースの作成に失敗しました。
goto end

:end
