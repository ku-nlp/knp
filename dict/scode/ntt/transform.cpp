#include<string>
#include<iostream>
#include<fstream>
#include<vector>
#include<map>
using namespace std;

/*
  
  入力ファイル(usr_word2code.dat)
  ===============================
  アスパラ 野菜:作物
  ...


  使い方
  ======
  
  % ./transform usr_word2code.dat sm2code.dat word2code.orig > word2code.dat
  
*/


map<string,string> SM2CODE, COPY;
map<string,int> ADD_LINE, DEL_LINE;

int read_usr_file( string filename );
void read_sm2code( string filename );
string sm2code ( string sm, string hinsi );
void write_word2code( string filename );
vector<string> tokenize( string line, string delm );
string tokenize( string line, string delm, int i );

int main ( int argc, char *argv[] ) {
    
    ifstream fin;
    string name, filename_in, filename_out;
    string line, word, code, hinsi_f;
    vector<string> sm_vec;
    int i;
    
    /*
      argv[1] => usr_word2code.dat
      argv[2] => sm2code.dat
      argv[3] => word2code.orig
    */
    if ( argc != 4 ) {
	exit(1);
    }
    
    // sm2code を読み込んで，データを SM2CODE に記憶する
    read_sm2code( argv[2] );

    // usr_word2code を読み込んで，word2code.orig に付加する行 ADD_LINE と，word2code.orig から削除する行 DEL_LINE を作成する
    read_usr_file( argv[1] );

    // word2code.dat を生成する
    write_word2code( argv[3] );
}



/*
  sm2code.dat を読み込む
*/
void read_sm2code( string filename ) {

    ifstream fin( filename.c_str() );
    string line, sm, code;

    if ( ! fin.is_open() ) {
	cerr << "Invalid file name: " << filename << "\n";
	exit(1);
    }
    
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
string sm2code ( string sm, string hinsi ) {

    string code;
    
    if ( SM2CODE.find( sm ) == SM2CODE.end() ) {
	return "NULL";
    }else {
	code = SM2CODE[sm];
    }

    // 品詞を変える
    code.replace( 0, 1, hinsi );

    return code;
}


/*
  usr_word2code を読み込んで，word2code.orig に付加する行 ADD_LINE と，word2code.orig から削除する行 DEL_LINE を作成する
*/
int read_usr_file( string filename ) {

    ifstream fin( filename.c_str() );
    string line, word, sm, code, hinsi;
    vector<string> tmp;
    int i, flag;

    if ( ! fin.is_open() ) {
	cerr << "no usr file name: " << filename << "\n";
	return 0;
    }
    
    while( getline( fin, line ) ) {

	if ( line.size() == 0 ) {
	}
	// アスパラ=アスパラガス
	else if ( line.find( " " ) == -1 ) {
	    COPY[tokenize( line, "=", 1 )] = tokenize( line, "=", 0 ); 
	}else {
	    tmp = tokenize( line, " " );
	    word = tmp[0];
	    
	    for ( i = 1; i < tmp.size(); i++ ) {
		sm = tmp[i];

		// 追加か削除かを調べる
		if ( sm.substr( 0, 1 ) == "-" ) {
		    flag = -1;
		    sm.erase( 0, 1 );
		}else {
		    flag = 1;
		}

		// 品詞を決める
		if ( ( sm.substr( 0, 1 ) == "3" ) || ( sm.substr( 0, 1 ) == "4" ) || ( sm.substr( 0, 1 ) == "5" ) || ( sm.substr( 0, 1 ) == "6" ) ||
		     ( sm.substr( 0, 1 ) == "7" ) || ( sm.substr( 0, 1 ) == "8" ) || ( sm.substr( 0, 1 ) == "9" ) || ( sm.substr( 0, 1 ) == "a" ) ||
		     ( sm.substr( 0, 1 ) == "b" ) || ( sm.substr( 0, 1 ) == "c" ) || ( sm.substr( 0, 1 ) == "d" ) || ( sm.substr( 0, 1 ) == "e" ) ||
		     ( sm.substr( 0, 1 ) == "f" ) || ( sm.substr( 0, 1 ) == "g" ) || ( sm.substr( 0, 1 ) == "h" ) || ( sm.substr( 0, 1 ) == "i" ) ||
		     ( sm.substr( 0, 1 ) == "j" ) || ( sm.substr( 0, 1 ) == "k" ) || ( sm.substr( 0, 1 ) == "l" ) || ( sm.substr( 0, 1 ) == "m" ) ||
		     ( sm.substr( 0, 1 ) == "n" ) ) {
		    hinsi = sm.substr( 0, 1 );
		    sm.erase( 0, 1 );
		}else {
		    hinsi = "3";
		}

		// code に変換する
		code = sm2code( sm, hinsi );

		if ( code == "NULL" ) {
		    cerr << "Invalid SM: " << sm << "\n";
		}else if ( flag == 1 ) {
		    ADD_LINE[word + " " + code] = 1;
		}else {
		    DEL_LINE[word + " " + code] = 1;
		}
	    }
	}
    }
    fin.close();
}


/*
  word2code.dat を生成する
*/
void write_word2code( string filename ) {

    ifstream fin( filename.c_str() );
    string line, word, code;
    map<string,int>::iterator it;

    if ( ! fin.is_open() ) {
	cerr << "Invalid file name: " << filename << "\n";
	exit(1);
    }

    while( getline( fin, line ) ) {
	word = tokenize( line, " ", 0 );
	code = tokenize( line, " ", 1 );

	if ( COPY.find( word ) != COPY.end() ) {
	    cout << COPY[word] << " " << code << "\n";
	}
	
	if ( ( DEL_LINE.find( line ) == DEL_LINE.end() ) && ( ADD_LINE.find( line ) == ADD_LINE.end() ) ) {
	    cout << line << "\n";
	}else {
	    //  ADD_LINE に入っている行は出力しない．下で出力する
	    //  DEL_LINE に入っている行は出力しない
	}
    }

    for ( it = ADD_LINE.begin(); it != ADD_LINE.end(); it++ ) {
	cout << it->first << "\n";
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
