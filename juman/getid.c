/*
==============================================================================
	getid.c
==============================================================================
*/

#include	<juman.h>

/*
------------------------------------------------------------------------------
	GLOBAL:
	definition of global variables
------------------------------------------------------------------------------
*/

extern CLASS	Class[CLASSIFY_NO + 1][CLASSIFY_NO + 1];
extern TYPE	Type[TYPE_NO];
extern FORM	Form[TYPE_NO][FORM_NO];

/*
------------------------------------------------------------------------------
	FUNCTION:
	<hinsi>: return <int:i> 
	         Class[i][0].id == x
------------------------------------------------------------------------------
*/

int get_hinsi_id(U_CHAR *x)
{
     int	i;

     if ( x == NULL ) 
       error(OtherError,"NULL string for hinsi.", EOA);

     if ( strcmp(x, "*") == 0 ) return 0;

     if ( strcmp(x, "Ï¢¸ì") == 0 ) return(atoi(RENGO_ID)); /* yamaji */

     for (i = 1; strcmp(Class[i][0].id, x); )
       if ( !Class[++i][0].id ) 
	 error(OtherError, (char *)x," is undefined in ", GRAMMARFILE, ".", EOA);
     
     return i;
}

/*
------------------------------------------------------------------------------
	FUNCTION:
	<bunrui>: return <int:i>
	          Class[hinsi][i].id == x
------------------------------------------------------------------------------
*/

int get_bunrui_id(U_CHAR *x, int hinsi)
{
     int	i;

     if ( !x ) 
       error(OtherError,"NULL string for bunrui.",  EOA);
     
     if ( strcmp(x, "*") == 0 ) return 0;

     if ( !Class[hinsi][1].id ) 
       error(OtherError, (char *)Class[hinsi][0].id, " has no bunrui in ",
	     GRAMMARFILE, ".", EOA);
     
     for ( i = 1; strcmp(Class[hinsi][i].id, x); )
       if ( !Class[hinsi][++i].id ) 
	 error(OtherError, (char *)Class[hinsi][0].id, " does not have bunrui ",
	       (char *)x, " in ", GRAMMARFILE, "." ,EOA);

     return i;
}

/*
------------------------------------------------------------------------------
	FUNCTION:
	<type>: return <int:i>
	        Type[i].name == x
------------------------------------------------------------------------------
*/

int get_type_id(U_CHAR *x)
{
     int	i;

     if ( !x ) 
       error(OtherError,"NULL string for type.",  EOA);

     if ( strcmp(x, "*") == 0 ) return 0;

     for (i = 1; strcmp(Type[i].name, x); )
       if ( !Type[++i].name ) 
	 error(OtherError, (char *)x, " is undefined in ", KATUYOUFILE, ".", EOA);

     return i;
}

/*
------------------------------------------------------------------------------
	FUNCTION:
	<type>: return <int:i>
	        Form[type][i].name == x
------------------------------------------------------------------------------
*/

int get_form_id(U_CHAR *x, int type)
{
     int	i;

     if ( !x ) 
       error(OtherError,"NULL string for form.",  EOA);

     if ( strcmp(x, "*") == 0 ) return 0;

     if ( type == 0 ) 
       error(OtherError, "Invalid type number for ", (char *)x, ".", EOA);

     for ( i = 1; strcmp(Form[type][i].name, x); )
       if ( !Form[type][++i].name )
	 error(OtherError, (char *)Type[type].name, " does not have katuyou ",
	       (char *)x, " in ", KATUYOUFILE, ".", EOA);
     
     return i;
}
