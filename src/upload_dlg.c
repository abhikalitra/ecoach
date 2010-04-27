/*
 *  eCoach
 *
 *  Copyright (C) 2010  Sampo Savola
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  See the file COPYING
 */

char* REQUEST_URI ="http://beta.moozement.net/oauth/request_token";
char* ACCESS_URI  ="http://beta.moozement.net/oauth/access_token";
char* AUTHORIZE_URI ="http://beta.moozement.net/oauth/authorize";
char* CONSUMER_KEY ="ngZfKFCJ2TtwvKBMXoQ";
char* CONSUMER_SECRET ="igR6ALGYUSlmU78APcw8CLtoNosBBXG6FhRlUdbP";


#include <gtk/gtk.h>
#include "upload_dlg.h"
#include <oauth.h>
void upload(AnalyzerView *data){


//if first time
first_time_authentication(data);

}

void first_time_authentication(AnalyzerView *data){


  get_request_token();

/*  GtkWidget *box;
  GtkWidget *dialog;
  dialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dialog), "Upload Settings");
  gtk_widget_set_size_request(dialog,800,300);
  
  gtk_dialog_run(GTK_DIALOG(dialog));
*/
}

void parse_reply(const char *reply, char *token, char *secret){

 int rc;

	char **rv = NULL;
	rc = oauth_split_url_parameters(reply, &rv);
	qsort(rv, rc, sizeof(char *), oauth_cmpstringp);
	if( rc==2
	&& !strncmp(rv[0],"oauth_token=",11)
	&& !strncmp(rv[1],"oauth_token_secret=",18) ) {
	token =strdup(&(rv[0][12]));
	secret=strdup(&(rv[1][19]));
	printf("key: '%s'\nsecret: '%s'\n",token, secret);
	}
 	if(rv) free(rv); 


}
int get_request_token(){
   
char *postarg = NULL; 
char *req_url = NULL;
char *reply = NULL;
char *token = NULL;
char *token_secret = NULL;

  req_url = oauth_sign_url2(REQUEST_URI, &postarg, OA_HMAC, NULL, CONSUMER_KEY, CONSUMER_SECRET,NULL, NULL);
  reply = oauth_http_post(req_url,postarg);
  if(reply != NULL){
  printf("REPLY: %s\n", reply);
  parse_reply(reply,token,token_secret);

// open browser for authentication 
 // system(/oauth/authorize&request_token=
  }
}














