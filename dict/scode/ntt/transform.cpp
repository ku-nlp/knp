#include<string.h>
#include<string>
#include<iostream>
#include<fstream>
#include<vector>
#include<map>

/*
  
  入力ファイル(*.dic)
  ====================
  アスパラ 野菜:作物
  ...

  
  出力ファイル(*.dat)
  ===================
  アスパラ 10211401****
  アスパラ 10201010****
  ...


  使い方
  ======
  
  % ls
  usr_add.rule usr_del.rule
  % ./transform usr_add
  % ls
  usr_add.rule usr_add.dat usr_del.rule
  % ./transform usr_del
  usr_add.rule usr_add.dat usr_del.rule usr_del.dat
  
*/
using namespace std;

map<string,string> SM2CODE;

void read_sm2code();
string sm2code ( string sm );
vector<string> tokenize( string line, string delm );
string tokenize( string line, string delm, int i );


int main ( int argc, char *argv[] ) {

  ifstream fin;
  ofstream fout;
  string name, filename_in, filename_out;
  string line, word, code, hinsi_f;
  vector<string> sm_vec;
  int i;

  if ( argc == 2 ) {
    name = argv[1];
    filename_in = name + ".dic";
    filename_out = name + ".dat";
  }else {
    exit(1);
  }
  read_sm2code();
  
  fin.open( filename_in.c_str() );
  fout.open( filename_out.c_str() );
  
  while( getline( fin, line ) ) {
    if ( line.size() == 0 ) {
      break;
    }
    word = tokenize( line, " ", 0 );
    sm_vec = tokenize( tokenize( line, " ", 1 ), ":" );
    
    for ( i = 0; i < sm_vec.size(); i++ ) {

	//  品詞を指定している場合
	if ( ( sm_vec[i].substr( 0, 1 ) == "3" ) ||
	     ( sm_vec[i].substr( 0, 1 ) == "4" ) ||
	     ( sm_vec[i].substr( 0, 1 ) == "5" ) ||
	     ( sm_vec[i].substr( 0, 1 ) == "6" ) ||
	     ( sm_vec[i].substr( 0, 1 ) == "7" ) ||
	     ( sm_vec[i].substr( 0, 1 ) == "8" ) ||
	     ( sm_vec[i].substr( 0, 1 ) == "9" ) ||
	     ( sm_vec[i].substr( 0, 1 ) == "a" ) ||
	     ( sm_vec[i].substr( 0, 1 ) == "b" ) ||
	     ( sm_vec[i].substr( 0, 1 ) == "c" ) ||
	     ( sm_vec[i].substr( 0, 1 ) == "d" ) ||
	     ( sm_vec[i].substr( 0, 1 ) == "e" ) ||
	     ( sm_vec[i].substr( 0, 1 ) == "f" ) ||
	     ( sm_vec[i].substr( 0, 1 ) == "g" ) ||
	     ( sm_vec[i].substr( 0, 1 ) == "h" ) ||
	     ( sm_vec[i].substr( 0, 1 ) == "i" ) ||
	     ( sm_vec[i].substr( 0, 1 ) == "j" ) ||
	     ( sm_vec[i].substr( 0, 1 ) == "k" ) ||
	     ( sm_vec[i].substr( 0, 1 ) == "l" ) ||
	     ( sm_vec[i].substr( 0, 1 ) == "m" ) ||
	     ( sm_vec[i].substr( 0, 1 ) == "n" ) ) {
	    sm_vec[i].erase( 0, 1 );
	    hinsi_f = sm_vec[i].substr( 0, 1 );
	}else {
	    hinsi_f = "3";  //  品詞を指定していなければ，デフォルト 3 
	}
      
	code = sm2code( sm_vec[i] );
	
	if ( code == "NULL" ) {
	    cerr << "invalid sm: " << sm_vec[i] << "\n";
	}else {
	    code.replace( 0, 1, hinsi_f );   // 品詞を変える
	    fout << word << " " << code << "\n";
	}
    }
  }
  fin.close();
  fout.close();
}



/*
  sm2code.dat を読み込む
*/
void read_sm2code() {

  ifstream fin( "sm2code.dat" );
  string line, sm, code;
  
  while( getline( fin, line ) ) {
    sm = tokenize( line, " ", 0 );
    code = tokenize( line, " ", 1 );
    SM2CODE[sm] = code;
  }
  fin.close();
}


/*
  sm を code に変換する
*/
string sm2code ( string sm ) {

  if ( SM2CODE.find( sm ) == SM2CODE.end() ) {
    return "NULL";
  }else {
    return SM2CODE[sm];
  }
}

vector<string> tokenize( string line, string delm ) {

  vector<string> token;
  int pos;

  while( ( pos = line.find( delm ) ) != -1 ) {
    token.push_back( line.substr( 0, pos ) );
    line.erase( 0, pos + 1 );
  }

  if ( line.size() != 0 ) {
    token.push_back( line );
  }
 
  return token;
}


string tokenize( string line, string delm, int i ) {

  vector<string> token = tokenize( line, delm );
  
  return token[i];
}
