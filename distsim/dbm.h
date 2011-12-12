#ifndef DBM_H
#define DBM_H

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <cdb.h>
#include <zlib.h>

using std::string;
using std::cout;
using std::cerr;
using std::endl;

class Dbm {
    bool compressed;
    bool available;
    bool defined_keymap;
    std::vector<std::pair<string, cdb*> > k2db;
    std::vector<cdb *> cdbs;

  public:
    Dbm(const string &in_dbname) {
	init(in_dbname, false);
    }
    Dbm(const string &in_dbname, bool compressed) {
	init(in_dbname, compressed);
    }

    ~Dbm() {
        if (defined_keymap) {
            for (std::vector<std::pair<string, cdb*> >::iterator it = k2db.begin(); it != k2db.end(); it++)
                free((*it).second);
        }
        else {
            for (std::vector<cdb *>::iterator it = cdbs.begin(); it != cdbs.end(); it++)
                free(*it);
        }
    }

    bool init(const string &in_dbname, bool _compressed) {
	compressed = _compressed;
	available = true;
	if (in_dbname.find("keymap") != string::npos) {
	    defined_keymap = true;

	    unsigned int loc = in_dbname.find_last_of("keymap");
	    string file0 = string(in_dbname, 0, loc - 6) + ".0";
            cdb *_db = tieCDB(file0);
            if (_db == NULL) {
                available = false;
            }
            k2db.push_back(std::pair<string, cdb*>("", _db));

	    string dirname = getDirName(in_dbname);
	    std::ifstream fin(in_dbname.c_str());
	    while (!fin.eof()) {
		string key;
		string filename;
		fin >> key;
		fin >> filename;
		if (filename.find("cdb") == string::npos)
		    break;

                cdb *_db = tieCDB(dirname + "/" + filename);
                if (_db == NULL) {
                    available = false;
                }
                k2db.push_back(std::pair<string, cdb*>(key, _db));
	    }
	    fin.close();
	}
	else {
	    defined_keymap = false;
	    cdb *_cdb = tieCDB(in_dbname);
            unsigned int dbcount = 0;
	    if (_cdb != NULL) {
                cdbs.push_back(_cdb);
                dbcount++;
            }

            // .1, .2, ...
            string dbname;
            while (1) {
                dbname = in_dbname + "." + int2string(dbcount);
                cdb *__cdb = tieCDB(dbname);
                if (__cdb == NULL) {
                    if (dbcount == 0)
                        available = false;
                    break;
                }
                else {
                    cdbs.push_back(__cdb);
                }
                dbcount++;
            }
	}

        if (available == false) {
            cerr << "Can't open DB: " << in_dbname << endl;
            exit(-1);
        }
	return available;
    }

    cdb* tieCDB(string dbfile) {
	cdb *db = (cdb*)malloc(sizeof(cdb));
	if (!db) {
	    cerr << "Can't allocate memory." << endl;
	    exit(-1);
	}

	int _fd;
	if ((_fd = open(dbfile.c_str(), O_RDONLY)) < 0) {
	    // cerr << "Can't open file: " << dbfile << endl;
            return NULL;
	}
	int ret = cdb_init(db, _fd);

	if (ret < 0) {
	    cerr << "Can't tie! " << dbfile << " " << ret << endl;
	}

	return db;
    }

    string getDirName(string filepath) {
	unsigned int loc = filepath.find_last_of("/");
	string dirname = string(filepath, 0, loc);
	return dirname;
    }

    bool is_open() {
	return available;
    }

    string get(string &key) {
	if (defined_keymap) {
	    return get_with_keymap(key);
	} else {
	    return get_without_keymap((const char*)key.c_str());
	}
    }

    string get(const char *key) {
	if (defined_keymap) {
	    string k = key;
	    return get_with_keymap(k);
	} else {
	    return get_without_keymap(key);
	}
    }

    string get_with_keymap(string &key) {
	cdb *db = k2db[0].second;
	for (std::vector<std::pair<string, cdb*> >::iterator it = k2db.begin(); it != k2db.end(); it++) {
	    if (key < (string)(*it).first) {
		break;
	    }
	    db = (*it).second;
	}

	string ret = get_using_specified_db((const char*)key.c_str(), db);
	return ret;
    }

    string get_without_keymap(const char *key) {
        string ret_value;
        for (std::vector<cdb *>::iterator it = cdbs.begin(); it != cdbs.end(); it++) {
            ret_value = get_using_specified_db(key, *it);
            if (ret_value.length() > 0) // found
                return ret_value;
        }
        return ret_value;
    }

    string get_using_specified_db(const char *key, cdb* db) {
	string ret_value;
	unsigned vlen, vpos;
	if (cdb_find(db, key, strlen(key)) > 0) {
	    vpos = cdb_datapos(db);
	    vlen = cdb_datalen(db);

	    if (compressed) {
		unsigned char *val = (unsigned char*)malloc(vlen);
		cdb_read(db, val, vlen, vpos);
		ret_value = decompress_string(val, vlen);
		free(val);
	    } else {
		// for \0
		char *val = (char*)malloc(vlen + 1);
		cdb_read(db, val, vlen, vpos);
		*(val + vlen) = '\0';
		ret_value = val;
		free(val);
	    }
	    // cout << key << " is found. val = " << val << endl;
	} else {
	    // cout << key << " is not found." << endl;
	}
	return ret_value;
    }

    string decompress_string(unsigned char *compressed, int len) {
	std::string out;
	int ret;
	char buffer[32768];
	z_stream zs;

	memset(&zs, 0, sizeof(zs));
	if (inflateInit(&zs) != Z_OK)
	    return out;

	zs.next_in = (Bytef*)compressed;
	zs.avail_in = len;

	do {
	    zs.next_out = reinterpret_cast<Bytef*>(buffer);
	    zs.avail_out = sizeof(buffer);

	    ret = inflate(&zs, Z_NO_FLUSH);
	    if (out.size() < zs.total_out) {
		out.append(buffer, zs.total_out - out.size());
	    }
	} while (ret == Z_OK);
	inflateEnd(&zs);

	if (ret != Z_STREAM_END) {
	    cerr << "Exception during zlib decompression: (" << ret << ") " << zs.msg << endl;
	    return "";
	}
	return out;
    }

};

#endif
