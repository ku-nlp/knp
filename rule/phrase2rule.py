#/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import re
import codecs
import warnings
from collections import defaultdict
from argparse import ArgumentParser
# from pyknp import KNP


######################################################################
#
#		 KNPの形態素，文節ルールのtranslator
#
#					99/09/10 by kuro@i.kyoto-u.ac.jp
######################################################################
#
# 各行のnotation
# ==============
#	[前の文脈]自分自身[後の文脈]\t+FEATURE列
#		※ FEATURE列の前は必ずTABで区切る
#		※ 前後の文脈，自分自身の中は文節ごとに空白で区切る
#						(文節ルールの場合)
#		※ FEATURE列はFEATUREごとに空白で区切る
#
#		※ 文節ルールの場合は前の文脈の前と後の文脈の後に
#		   任意の文節列を許す(?*が自動挿入される)
#		※ 形態素ルールの場合は前の文脈の前と後の文脈の後に
#		   任意の形態素列を許す(?*が自動挿入される)
#
# 形態素，文節のnotation
# ======================
#	^....		文節先頭からマッチ (文節ルールの場合)
#	<<...>>		文節のfeature
#	<...>		直前の形態素のfeature (〜<...>とすればfeatureだけの指定)
#
#	(...)		0回以上の出現 (形態素レベルと文節レベルそれぞれ)
#	{WORD1|WORD2|..}WORD1 or WORD2 or ...
#			  (扱えるのは品詞が同一の場合だけ)
#
#	{\品詞}		その品詞
#	{\品詞:細分類}	その細分類
#
#	‥		任意の形態素列 (※ ……は記号一語で使っている)
#	〜		任意の形態素 (形態素解析のために挿入，ルールでは削除)
#
#	WORD_活用語G	任意の活用語 (任意の活用形)
#	WORD_非活用語G	任意の非活用語
#
#	WORD_活用語g	品詞,細分類:*，活用型:以下の一般化，活用形:そのまま，語:*
#			  動詞タイプ 例) 書くg
#                         --------------------
#				母音動詞 子音動詞＊行 カ変動詞 サ変動詞 ザ変動詞
#				動詞性接尾辞ます型 動詞性接尾辞うる型
#				無活用型
#			  イ形容詞タイプ 例) 美しいg
#                         -------------------------- 
#			 	イ形容詞＊段
#				助動詞ぬ型 助動詞く型
#			  ナ形容詞タイプ 例) 静かだg
#                         --------------------------
#				ナ形容詞 ナ形容詞特殊 ナノ形容詞 判定詞
#				助動詞ぬ型 助動詞だろう型 助動詞そうだ型
#				(「特別の」にすればダ列特殊連体となる)
#				(※ 「助動詞ぬ型」はイ形容詞とナ形容詞両方)
#			  タル形容詞タイプ 例) 堂々たるg
#                         ------------------------------
#				タル形容詞
#
#	WORD_非活用語g	名詞，名詞性名詞接尾辞，名詞性述語接尾辞
#
#	WORD12345	品詞，細分類，活用型，活用形，語について数字指定の部分だけのこす
#			  例) する1235 --> 活用形だけ何でもよい
#
#	※例外事項※	・"#define WORD 般化予約語"とした場合はWORDgと解釈
#			・句読点，括弧はgなしでも語の一般化を行う
#			・名詞，副詞，助詞は常に細分類を一般化

# 活用型の対応付け
# -----------------------------------------
# 無活用型		動詞
# 助動詞ぬ型		イ形容詞 ナ形容詞
# 助動詞く型		イ形容詞
# 動詞性接尾辞ます型	動詞
# 動詞性接尾辞うる型	動詞

conj_type = ["", "", "", ""]
conj_type[0] = "母音動詞 子音動詞カ行 子音動詞カ行促音便形 子音動詞ガ行 子音動詞サ行 子音動詞タ行 子音動詞ナ行 子音動詞バ行 子音動詞マ行 子音動詞ラ行 子音動詞ラ行イ形 子音動詞ワ行 子音動詞ワ行文語音便形 カ変動詞 カ変動詞来 サ変動詞 ザ変動詞 動詞性接尾辞ます型 動詞性接尾辞うる型 無活用型"
conj_type[1] = "イ形容詞アウオ段 イ形容詞イ段 イ形容詞イ段特殊 助動詞ぬ型 助動詞く型"
conj_type[2] = "ナ形容詞 ナ形容詞特殊 ナノ形容詞 判定詞 助動詞ぬ型 助動詞だろう型 助動詞そうだ型"
conj_type[3] = "タル形容詞"

# 本当は全体に書いておく方がよいが，とりあえず使われるものだけ
pos_repr = defaultdict(str)
pos_repr["\\名詞:形式名詞"] = "こと"
pos_repr["\\名詞:副詞的名詞"] = "ため"
pos_repr["\\接尾辞"] = "個"
pos_repr["\\接尾辞:名詞性名詞助数辞"] = "個"
pos_repr["\\接尾辞:名詞性特殊接尾辞"] = "以内"
pos_repr["\\助詞"] = "だけ"
pos_repr["\\助詞:格助詞"] = "に"
pos_repr["\\助詞:副助詞"] = "だけ"
pos_repr["\\指示詞:名詞形態指示詞"] = "これ"
pos_repr["\\指示詞:副詞形態指示詞"] = "こう"
pos_repr["\\連体詞"] = "ほんの"
pos_repr["\\指示詞:連体詞形態指示詞"] = "この"
pos_repr["\\接続詞"] = "そして"
pos_repr["\\感動詞"] = "あっ"
pos_repr["\\特殊:句点"] = "．"

parser = ArgumentParser()
parser.add_argument("--rid", dest="rid", metavar="INT", type=str, help="RIDを付与")
args = parser.parse_args()

bnstrule_flag = 1
num = 0
g_define_word = defaultdict(int)

######################################################################
try:
    from pyknp import KNP
except:
    warnings.warn("*** Cannot find pyknp package")
    sys.exit(0)
knp = KNP(jumanpp=True)
######################################################################

def bnst_cond(input, flag, l_context, r_context):
    # defaultdictのままだと困るのでデータ書き換え
    if type(l_context) == defaultdict:
        l_context = ""
    if type(r_context) == defaultdict:
        r_context = ""

    # 文節の条件を処理 (flag=1:通常の処理，flag=0:代表表現をかえす)
    ast_flag = ""
    string = ""
    feature = ""

    # (....) -> ....とast_flagに分離
    search1 = re.search(r"^\((.+)\)$", input)
    if bnstrule_flag and search1:
        input = search.group(1)
        ast_flag = 1
    else:
        ast_flag = 0

    # 文節のFEATUREを分離
    search2 = re.search("^<<([^<>]+)>>$", input)
    search3 = re.search("^(.+)<<([^<>]+)>>$", input)
    if search2:
        string = "‥"
        feature = search2.group(1)
    elif search3:
        string = search3.group(1)
        feature = search3.group(2)
    else:
        string = input
        feature = ""

    # 出力
    if flag:
        if bnstrule_flag:
            print(" < (", end="")
            bnst_cond2(string, 1, l_context, r_context)
            print(" ){0} >".format(feature2str(feature)), end="")
            if ast_flag:
                print("*", end="")
        else:
            bnst_cond2(string, 1, l_context, r_context);
    else:
        return bnst_cond2(string, 0, "", "")
    
######################################################################
def feature2str(input):
    if not input:
        return ""

    data = " ("
    split_input = input.split("\|\|")
    for elem in split_input:        
        data += "(" + re.sub("\&\&", " ", elem) + ")"
    data += ")"
    return data

######################################################################
def bnst_cond2(input, flag, l_context, r_context):
    # 文節の条件を処理 (flag=1:通常の処理，flag=0:代表表現をかえす)

    # (彼|彼女)に(だけ|すら|さえ)は
    #
    # part: [0][0]:彼   [1][0]:に [2][0]:だけ [3][0]:は
    #       [0][1]:彼女           [2][1]:すら
    #                             [2][1]:さえ
    # 
    # data[0][0]{phrase}: 彼にだけは
    # data[0][1]{phrase}: 彼女にだけは
    # data[2][1]{phrase}: 彼にすらは
    # data[2][2]{phrase}: 彼にさえは
    # 
    # ※ data[0][0]{phrase}はすべてのpartで0番目の語を集めたもの
    #    data[i][j]{phrase}はi番目のpartをj番目の語にしたもの

    data = defaultdict(lambda : defaultdict(lambda : defaultdict(lambda : defaultdict())))
    feature = defaultdict(dict)
    part = defaultdict(list)
    mrph = defaultdict(lambda : defaultdict(lambda : defaultdict(lambda : defaultdict())))
    i = ""
    j = ""
    k = ""
    l = ""
    error_flag = None
    any_prefix = None
    knp_input = None

    # 先頭が ^ であれば先頭からの条件
    if re.search("^\^", input) or bnstrule_flag == 0:
        input = re.sub("^\^", "", input)
        any_prefix = 0;
    else:
        any_prefix = 1;

    # 要素ごとに分割 --> @part
    # (括弧,‥の前後，>,G,g,数字の後に空白を入れる)
    input = re.sub("\(", u" (", input)
    input = re.sub("\)", u") ", input)
    input = re.sub("\{", u" {", input)
    input = re.sub("\}", u"} ", input)
    input = re.sub("‥", " ‥ ", input)
    input = re.sub(">", "> ", input)
    input = re.sub("G", "G ", input)
    input = re.sub("g", "g ", input)

    input = re.sub("(\d+)", r"\1 ", input)
    input = re.sub("(\d) (\))", r"\1\2", input)
    input = re.sub("(\d) (<)", r"\1\2", input)

    input = re.sub("^ +| +$", "", input)
    if input != "":
        # inputが空のケースもあるため
        part_str = re.split(" +", input);
        part_num = len(part_str)
    else:
        part_num = 0
        
    # part_strを整形し，part(2次元配列)を作成
    for i in range(0, part_num):
        search1 = re.search("^\((.+)\)$", part_str[i])
        search2 = re.search("^{(.+)}$", part_str[i])
        if search1:
            part_str[i] = search1.group(1)
            feature[i]["ast"] = 1;
        elif search2:
            part_str[i] = search2.group(1)

	# ORの場合は表層表現だけ
        if not re.search("^<<", part_str[i]) and re.search("\|", part_str[i]):
            part[i] = part_str[i].split("|")
            feature[i]["or"] = 1

	# 他は種々のメタ表現を考慮
        else:
            part[i].append(part_str[i])
            search3 = re.search("<(.+)>$", part[i][0])
            if search3:
                feature[i]["lastfeature"] = feature2str(search3.group(1))
                part[i][0] = re.sub("<.+>$", "", part[i][0])
            if part[i][0] == "‥":
                feature[i]["result"] = " ?*"
                part[i][0] = "‥"
            if re.search("G$", part[i][0]):
                feature[i]["lastGENERAL"] = 1
                part[i][0] = re.sub("G$", "", part[i][0])
            if re.search("g$", part[i][0]):
                feature[i]["lastgeneral"] = 1
                part[i][0] = re.sub("g$", "", part[i][0])
            search4 = re.search(r"([\d]+)$", part[i][0])
            if search4:
                feature[i]["lastnum"] = search4.group(1)
                part[i][0] = re.sub("[\d]+$", "", part[i][0])
            if re.search(r"^\\", part[i][0]):
                feature[i]["result"] = " [" + part[i][0] + "]"
                feature[i]["result"] = re.sub(r"\\", "", feature[i]["result"])
                feature[i]["result"] = re.sub("\:", " ", feature[i]["result"])
                part[i][0] = pos_repr[part[i][0]]

    # $part[$i][0]を標準データとして$data[0][0]に
    for i in range(0, part_num):
        if i == 0:
            data[0][0]["phrase"] = part[i][0]
        else:
            data[0][0]["phrase"] = data[0][0]["phrase"] + part[i][0]
        data[0][0]["length"][i] = len(part[i][0])

    # 代表表現を返す場合
    if flag == 0:
        return data[0][0]["phrase"]

    # i番目のpartをj番目の語にしたものを$data[i][j]に
    for i in range(0, part_num):
        if not "or" in feature[i]:
            continue
        for j in range(1, len(part[i])):
            for k in range(0, part_num):
                if k != i:
                    if "phrase" in data[i][j]:
                        data[i][j]["phrase"] += part[k][0]                    
                    else:
                        data[i][j]["phrase"] = part[k][0]
                    data[i][j]["length"][k] = len(part[k][0])
                else:
                    if "phrase" in data[i][j]:
                        data[i][j]["phrase"] += part[k][j]
                    else:
                        data[i][j]["phrase"] = part[k][j]
                    data[i][j]["length"][k] = len(part[k][j])

    # それぞれKNP(JUMAN)で処理
    for i in range(0, part_num):
        for j in range(0, len(part[i])):
            if not data[i][j]:
                continue
            # 末尾が"，|．"なら"＝"，それ以外なら"，＝"を付与
            # ※ 以下の問題を解決するため
            #      1. 連体形の文節がそれだけでは正しくJUMANされない
            #      2. 単に"＝"を付与すると"同じ"が語幹になる
            #      2. "，，"はJUMANで未定義語
            #      3. 末尾の"，"もJUMANで未定義語
            knp_input = l_context + data[i][j]["phrase"] + r_context
            if bnstrule_flag:
                if re.search("(，|．|、|。)$", knp_input):
                    knp_input += "＝"
                else:
                    knp_input += "，＝"
            try:
                result = knp.parse(knp_input)
            except:
                warnings.warn("*** Cannot get the result of KNP to update a rule file! ***")
                sys.exit(0)
            knp_result = re.sub(r"\n\n", "\n", result.spec().strip()).split("\n")
	    # print("\n\n>>>>>{0}\n>>>{1}".format(knp_input, knp_result));

            k = 0
            for line in knp_result:
                if re.search("^EOS", line):
                    continue
                if re.search("^(\*|\#|\;|\+)", line):
                    continue
                mrph_split = line.strip().split(" ")
                mrph[i][j][k]["word"] = mrph_split[0]
                mrph[i][j][k]["yomi"] = mrph_split[1]
                mrph[i][j][k]["base"] = mrph_split[2]
                mrph[i][j][k]["pos"] = mrph_split[3]
                mrph[i][j][k]["d1"] = mrph_split[4]
                mrph[i][j][k]["pos2"] = mrph_split[5]
                mrph[i][j][k]["d2"] = mrph_split[6]
                mrph[i][j][k]["conj"] = mrph_split[7]
                mrph[i][j][k]["d3"] = mrph_split[8]
                mrph[i][j][k]["conj2"] = mrph_split[9]
                mrph[i][j][k]["d4"] = mrph_split[10]
                mrph[i][j][k]["length"] = len(mrph[i][j][k]["word"])
                mrph[i][j][k]["result"] = " [" + mrph[i][j][k]["pos"] + " " + mrph[i][j][k]["pos2"] + " " + mrph[i][j][k]["conj"] + " " + mrph[i][j][k]["conj2"] + " " + mrph[i][j][k]["base"] + "]"
                k += 1

	    # l_contextとr_contextが形態素として正しく区切れているかチェック
            begin_check = 0
            end_check = 0
            length = 0            
            for k in range(0, len(mrph[i][j])):                
                if length == len(l_context):
                    begin_check = 1
                    mrph_start_num = k
                length += mrph[i][j][k]["length"]
                if length == len(l_context) + len(data[i][j]["phrase"]):
                    end_check = 1
            if not begin_check or not end_check:
                print("CONTEXT ERROR ({0})".format(pattern), file = sys.stderr)
                return

            # part と mrph の対応つけ 
            # (場合によってORの文字数が違うので各$data[$i][$j]に必要)
            part_length = 0
            mrph_length = 0
            k = mrph_start_num
            for l in range(0, part_num):
                part_length += data[i][j]["length"][l]
                data[i][j]["start"][l] = k
                while mrph_length < part_length:
                    mrph_length += mrph[i][j][k]["length"]
                    k += 1
                data[i][j]["end"][l] = k - 1
                # print("({0} {1})".format(data[i][j]["start"][l], data[i][j]["end"][l]))

    # ORの部分のマージ
    start_pos = 0;
    error_flag = 0;
    for i in range(0, part_num):
        WORD = ""
        if "or" in feature[i]:
            for j in range(1, len(part[i])):
	        # ORの前後の形態素数の一致， ORが一形態素
                if data[0][0]["start"][i] != data[i][j]["start"][i] or data[0][0]["end"][i] != data[i][j]["end"][i] or data[0][0]["start"][i] != data[0][0]["end"][i] or data[i][j]["start"][i] != data[i][j]["end"][i] or len(mrph[0][0]) != len(mrph[i][j]):
                    error_flag = 1
		# ORの前の形態素列の一致
                for k in range(0, data[0][0]["start"][i]):
                    if mrph[0][0][k]["result"] != mrph[i][j][k]["result"]:
                        error_flag = 1
                        break
		# ORの後の形態素列の一致
                for k in range(data[0][0]["end"][i]+1, len(mrph[0][0])):
                    if mrph[0][0][k]["result"] != mrph[i][j][k]["result"]:
                        error_flag = 1
                        break
                # ORが同一品詞か
                if mrph[0][0][data[0][0]["start"][i]]["pos"] != mrph[i][j][data[i][j]["start"][i]]["pos"]:
                    error_flag = 1
                    if mrph[0][0][data[0][0]["start"][i]]["conj"] == "*" and mrph[i][j][data[i][j]["start"][i]]["conj"] != "*":
			# $data[0][0]を標準とするので，$data[0][0]が無活用，$data[i][j]が活用の場合
			# $data[i][j]のwordをbaseにコピーする                        
                        mrph[i][j][data[i][j]["start"][i]]["base"] = mrph[i][j][data[i][j]["start"][i]]["word"]
                if j == 1:
                    WORD = mrph[0][0][data[0][0]["start"][i]]["base"] 
                WORD = WORD + " " + mrph[i][j][data[i][j]["start"][i]]["base"]
	    # print(WORD)
            mrph[0][0][data[0][0]["start"][i]]["base"] = "(" + WORD + ")"
        start_pos += data[0][0]["length"][i]

    # ORの条件を満たさなければエラー出力
    if error_flag:
        for i in range(0, part_num):
            for j in range(0, len(part[i])):
                if not data[i][j]:
                    continue
                print("ERROR({0},{1}) ".format(i, j), file=sys.stderr, end="")
                for k in range(0, len(mrph[i][j])):
                    print(mrph[i][j][k]["result"], file=sys.stderr, end="")
                print("", file=sys.stderr);
        print("", file=sys.stderr);

    # 出力
    if any_prefix and part[0][0] != "‥":
        print(" ?*", end="")
    for i in range(0, part_num):
        if "result" in feature[i]:
            print(feature[i]["result"], end="")
            if "ast" in feature[i]:
                print("*", end="")
        else:
            for k in range(data[0][0]["start"][i], data[0][0]["end"][i] + 1):
                if k == data[0][0]["end"][i] and "lastGENERAL" in feature[i]:
                    if mrph[0][0][k]["conj"] != "*":
                        mrph[0][0][k]["result"] = " [* * * * * ((活用語))]"
                    else:
                        mrph[0][0][k]["result"] = " [* * * * * ((^活用語))]"
                elif (k == data[0][0]["end"][i] and "lastgeneral" in feature[i]) or g_define_word[mrph[0][0][k]["base"]]:
                    if mrph[0][0][k]["conj"] != "*":
                        conj_flag = 0
                        for m in range(0, len(conj_type)):
                            if re.search(mrph[0][0][k]["conj"], conj_type[m]):
                                mrph[0][0][k]["result"] = " [* * (" + conj_type[m] + " " + mrph[0][0][k]["conj2"] + " *]"
                                conj_flag = 1
                                break
                        if not conj_flag:
                            print("Invalid conjugation type ({0})!!".format(mrph[0][0][k]["conj"]), file=sys.stderr)
                    else:
                        mrph[0][0][k]["result"] = " [* * * * * ((名詞相当語))]"
                elif k == data[0][0]["end"][i] and "lastnum" in feature[i]:		
                    if re.search("1", feature[i]["lastnum"]):
                        mrph[0][0][k]["result"] = " [" + mrph[0][0][k]["pos"]
                    else:
                        mrph[0][0][k]["result"] = " [*"
                    if re.search("2", feature[i]["lastnum"]):
                        mrph[0][0][k]["result"] = mrph[0][0][k]["result"] + " " + mrph[0][0][k]["pos2"]
                    else:
                        mrph[0][0][k]["result"] = mrph[0][0][k]["result"] + " *"
                    if re.search("3", feature[i]["lastnum"]):
                        mrph[0][0][k]["result"] = mrph[0][0][k]["result"] + " " + mrph[0][0][k]["conj"]
                    else:
                        mrph[0][0][k]["result"] = mrph[0][0][k]["result"] + " *"
                    if re.search("4", feature[i]["lastnum"]):
                        mrph[0][0][k]["result"] = mrph[0][0][k]["result"] + " " + mrph[0][0][k]["conj2"]
                    else:
                        mrph[0][0][k]["result"] = mrph[0][0][k]["result"] + " *"
                    if re.search("5", feature[i]["lastnum"]):
                        mrph[0][0][k]["result"] = mrph[0][0][k]["result"] + " " + mrph[0][0][k]["base"] + "]"
                    else:
                        mrph[0][0][k]["result"] = mrph[0][0][k]["result"] + " *]"
                elif mrph[0][0][k]["base"] == "，":
                    mrph[0][0][k]["result"] = " [特殊 読点 * * *]"
                elif mrph[0][0][k]["base"] == "．":
                    mrph[0][0][k]["result"] = " [特殊 句点 * * *]"
                elif mrph[0][0][k]["base"] == "「":		
                    mrph[0][0][k]["result"] = " [特殊 括弧始 * * *]"
                elif mrph[0][0][k]["base"] == "」":
                    mrph[0][0][k]["result"] = " [特殊 括弧終 * * *]"
                elif mrph[0][0][k]["base"] == "〜":
                    mrph[0][0][k]["result"] = ""
                else:
                    if (mrph[0][0][k]["pos"] == "名詞"):
                        mrph[0][0][k]["pos2"] = "*"
                    if (mrph[0][0][k]["pos"] == "副詞"):
                        mrph[0][0][k]["pos2"] = "*"
                    if (mrph[0][0][k]["pos"] == "助詞"):
                        mrph[0][0][k]["pos2"] = "*"
                    mrph[0][0][k]["result"] = " [" + mrph[0][0][k]["pos"] + " " + mrph[0][0][k]["pos2"] + " " + mrph[0][0][k]["conj"] + " " + mrph[0][0][k]["conj2"] + " " + mrph[0][0][k]["base"] + "]"

		# 形態素のfeatureが指定されている場合
                if k == data[0][0]["end"][i] and "lastfeature" in feature[i]:
                    if mrph[0][0][k]["result"]:
                        mrph[0][0][k]["result"] = re.sub("\]$", feature[i]["lastfeature"] + "]", mrph[0][0][k]["result"])
                    else:
                        mrph[0][0][k]["result"] = " [* * * * *" + feature[i]["lastfeature"] + "]";
                print(mrph[0][0][k]["result"], end="")
                if "ast" in feature[i]:
                    print("*", end="")
            if "ast" in feature[i] and data[0][0]["end"][i] - data[0][0]["start"][i] > 0:
                print("{0}: *は各形態素につくだけ！！！".format(input), file=sys.stderr)

######################################################################
if __name__ == "__main__":
    for line in sys.stdin.readlines():
        strip_line = line.strip()
        num += 1
        if re.search(r"^[\s\t]*\;", line.strip())  or len(strip_line) == 0:
            continue
    
        if re.search(r"^\#define", strip_line):
            tmp = strip_line.split()
            if tmp[1] == "文節ルール":
                bnstrule_flag = 1
            elif tmp[1] == "形態素ルール":
                bnstrule_flag = 0
            elif tmp[2] == "般化予約語":
                g_define_word[tmp[1]] = 1
            else:
                warnings.warn("Invalid define line {0}!!".format(strip_line))
                sys.exit(1)
            continue

        # 表現とFEATURE列とコメントを分離
        # 	FEATURE列との間はtab，コメントは ; の後
        search1 = re.search(r"^([^\t]+)[\s\t]+([^\;\t]+)[\s\t]*(\;.+)$", strip_line)
        search2 = re.search(r"^([^\t]+)[\s\t]+([^\;\t]+)[\s\t]*$", strip_line) 
        if search1:
            pattern = search1.group(1)
            feature = search1.group(2)
            comment = search1.group(3)
        elif search2:
            pattern = search2.group(1)
            feature = search2.group(2)
            comment = "";
        else:
            print("line {0} is invalid; {1}".format(num, strip_line), file=sys.stderr);
            continue
        pattern = re.sub("^\s+", "", pattern)	# 念のため
        pattern = re.sub("\s+$", "", pattern)	# 念のため
    
        # 前後の文脈と自分自身を分離，前後の文脈は[...]
        search3 = re.search("^(\[[^\[\]]+\])?([^\[\]]+)(\[[^\[\]]+\])?$", pattern)
        pres = re.sub("^\[|\]$", "", search3.group(1)).split(" ") if search3.group(1) else []
        self = re.sub("^\[|\]$", "", search3.group(2)).split(" ") if search3.group(2) else []
        poss = re.sub("^\[|\]$", "", search3.group(3)).split(" ") if search3.group(3) else []
        # print(">>pre  {0}\n self {1}\n post {2}\n".format(pres, self, poss))
    
        # 前後の文脈の前後には空文字列を挿入(出力時は?*)    
        if bnstrule_flag:
            if len(pres) == 0 or pres[0] != "‥":
                pres.insert(0, "")
            if len(poss) == 0 or poss[-1] != "‥":
                poss.append("")
        all = pres + self + poss
    
        # 代表句をつくる
        repr_str = []
        for elem in all:
            repr_str.append(bnst_cond(elem, 0, "", ""))

        # MAIN        
        print("; {0}".format(pattern))
        print("(\n(", end="")
        # 前の文脈
        for i in range(0, len(pres)):
            if bnstrule_flag:
                if i == 0:
                    print(" ?*", end="")
                else:
                    bnst_cond(all[i], 1, repr_str[i-1], repr_str[i+1])
            else:
                print(" ?*", end="")
                bnst_cond(all[i], 1, "", repr_str[i+1])
        print(" )\n(", end="")
        # 自分
        for i in range(len(pres), len(pres) + len(self)):
            bnst_cond(all[i], 1, repr_str[i-1], repr_str[i+1])
        print(" )\n(", end="")
        # 後の文脈
        for i in range(len(pres) + len(self), len(all)):
            if bnstrule_flag:
                if i == len(all) - 1:
                    print(" ?*", end="")
                else:
                    bnst_cond(all[i], 1, repr_str[i-1], repr_str[i+1])
            else:
                bnst_cond(all[i], 1, repr_str[i-1], "")
                print(" ?*", end="")
        print(" )\n\t{0}".format(feature), end="")
        if args.rid:
            print(" ＴRID:{0}".format(args.rid) , end="")
        print("\n)");
        if comment:
            print(comment) 
        print("")
