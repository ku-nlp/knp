SUBDIRS = src

EXTRA_DIST = Makefile.orig


include Makefile.orig

make_dir :
	if [ ! -d bgh ]; then \
		mkdir bgh; \
	fi
	if [ ! -d scase ]; then \
		mkdir scase; \
	fi
	if [ ! -d cf ]; then \
		mkdir cf; \
	fi
	if [ ! -d sm ]; then \
		mkdir sm; \
	fi


database_ipal:
	if [ -f "$(IPALV1_FILE)" -a "$(IPALV2_FILE)" ]; then \
		cat $(IPALV1_FILE) $(IPALV2_FILE) | src/ipalv.pl > cf/ipalv.dat; \
	fi
	if [ -f "$(IPALA1_FILE)" -a "$(IPALA2_FILE)" ]; then \
		src/ipala.pl $(IPALA1_FILE) $(IPALA2_FILE) > cf/ipala.dat; \
	fi
	if [ -n "$(IPALN_DIR)" -a -d "$(IPALN_DIR)" ]; then \
		cat $(IPALN_DIR)/n31.* | src/ipals.pl | src/ipals_add_voice.pl > cf/ipals.dat; \
	fi
	if [ -f cf/ipalv.dat -o -f cf/ipals.dat -o -f cf/ipala.dat ]; then \
		cat cf/ipalv.dat cf/ipals.dat cf/ipala.dat | $(CF_CMD) cf/ipal.idx cf/ipal.dat; \
		cat cf/ipalv.dat cf/ipals.dat cf/ipala.dat | src/ipal2scase.pl >> scase/scase.dat; \
	fi


make_hashdb: hashdb_bgh hashdb_sm2bghcode hashdb_sm hashdb_scase hashdb_cf

hashdb_sm: hashdb_word2code hashdb_sm2code hashdb_code2sm hashdb_smp2smg


bgh/bgh.dat:
	if [ ! -f bgh/bgh.dat -a -f "$(BGH_FILE)" ]; then \
		nkf -S -e < "$(BGH_FILE)" | src/bgh.pl > bgh/bgh.dat; \
	fi

bgh/bgh$(DBEXT): bgh/bgh.dat
	if [ -f bgh/bgh.dat ]; then \
		$(MAKE_DB) $(TEMPDIR)/bgh < bgh/bgh.dat; \
		mv $(TEMPDIR)/bgh bgh/bgh$(DBEXT); \
	fi

hashdb_bgh: bgh/bgh$(DBEXT)


$(SM2BGHCODE):
	true

bgh/sm2code$(DBEXT): $(SM2BGHCODE)
	if [ -f $(SM2BGHCODE) ]; then \
		$(MAKE_DB) $(TEMPDIR)/sm2code < $(SM2BGHCODE); \
		mv $(TEMPDIR)/sm2code bgh/sm2code$(DBEXT); \
	fi

hashdb_sm2bghcode: bgh/sm2code$(DBEXT)


$(WORD2CODE):
	true

sm/word2code$(DBEXT): $(WORD2CODE)
	if [ -f $(WORD2CODE) ]; then \
		$(MAKE_DB) $(TEMPDIR)/word2code < $(WORD2CODE); \
		mv $(TEMPDIR)/word2code sm/word2code$(DBEXT); \
	fi

hashdb_word2code: sm/word2code$(DBEXT)


$(SM2CODE):
	true

sm/sm2code$(DBEXT): $(SM2CODE)
	if [ -f $(SM2CODE) ]; then \
		$(MAKE_DB) $(TEMPDIR)/sm2code < $(SM2CODE); \
		mv $(TEMPDIR)/sm2code sm/sm2code$(DBEXT); \
	fi

hashdb_sm2code: sm/sm2code$(DBEXT)


$(CODE2SM):
	true

sm/code2sm$(DBEXT): $(CODE2SM)
	if [ -f $(CODE2SM) ]; then \
		$(MAKE_DB) $(TEMPDIR)/code2sm < $(CODE2SM); \
		mv $(TEMPDIR)/code2sm sm/code2sm$(DBEXT); \
	fi

hashdb_code2sm: sm/code2sm$(DBEXT)


$(SMP2SMG):
	true

sm/smp2smg$(DBEXT): $(SMP2SMG)
	if [ -f $(SMP2SMG) ]; then \
		$(MAKE_DB) $(TEMPDIR)/smp2smg -append / < $(SMP2SMG); \
		mv $(TEMPDIR)/smp2smg sm/smp2smg$(DBEXT); \
	fi

hashdb_smp2smg: sm/smp2smg$(DBEXT)


scase/scase.dat:
	if [ ! -f scase/scase.dat -a -f "$(EDR_FILE)" ]; then \
		src/edr.pl < "$(EDR_FILE)" > scase/scase.dat; \
	fi

scase/scase$(DBEXT): scase/scase.dat
	if [ -f scase/scase.dat ]; then \
		$(MAKE_DB) $(TEMPDIR)/scase -or < scase/scase.dat; \
		mv $(TEMPDIR)/scase scase/scase$(DBEXT); \
	fi

hashdb_scase: scase/scase$(DBEXT)


cf/cf.idx:
	if [ -f cf/cf.knpdict ]; then \
		$(CF_CMD) cf/cf.idx cf/cf.dat < cf/cf.knpdict; \
	fi

cf/cf$(DBEXT): cf/cf.idx
	if [ -f cf/cf.idx ]; then \
		$(MAKE_DB) $(TEMPDIR)/cf -append / < cf/cf.idx; \
		mv $(TEMPDIR)/cf cf/cf$(DBEXT); \
	fi

hashdb_cf: cf/cf$(DBEXT)


hashdb_rsk:
	if [ -f rsk/rsk.dat ]; then \
		$(MAKE_DB) $(TEMPDIR)/rsk -append / < rsk/rsk.dat; \
		mv $(TEMPDIR)/rsk rsk/rsk$(DBEXT); \
	fi


all: make_dir make_hashdb

clean: clean_db

clean_db:
	rm -f bgh/bgh$(DBEXT)
	rm -f scase/scase$(DBEXT)
	rm -f sm/word2code$(DBEXT) sm/sm2code$(DBEXT) sm/code2sm$(DBEXT) sm/smp2smg$(DBEXT)
	rm -f rsk/rsk$(DBEXT)
	rm -f cf/ipal$(DBEXT)
