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

char* REQUEST_URI ="http://www.heiaheia.com/oauth/request_token";
char* ACCESS_URI  ="http://www.heiaheia.com/oauth/access_token";
char* AUTHORIZE_URI ="http://www.heiaheia.com/oauth/authorize";
char* CONSUMER_KEY ="wHaXs4sT0AVMtiLVH2uU";
char* CONSUMER_SECRET ="Dp2pQAkIP5ALPcmAIZsSTWVTPomUoFwZGKehX0zY";

char *TOKEN;
char *TOKEN_SEC;

#include "debug.h"
#include <gtk/gtk.h>
#include "upload_dlg.h"
#include <oauth.h>
#include <string.h>
#include <stdlib.h>
#include <hildon/hildon.h>

static void authorized_clicked(GtkWidget *button,gpointer user_data);
static int get_request_token(AnalyzerView *data);
static void  show_information_note(GtkWidget *parent);

void upload(AnalyzerView *data){
    DEBUG_BEGIN();
    data->status = 1;

   DEBUG("Filename %s",data->filename);
   gchar *gpx;
   g_file_get_contents(data->filename,&gpx,NULL,NULL);



    /*Check if it is the first time and get tokens */
    TOKEN = gconf_helper_get_value_string_with_default(data->gconf_helper,TOKEN_KEY,"");
    TOKEN_SEC = gconf_helper_get_value_string_with_default(data->gconf_helper,TOKEN_SECRET_KEY,"");
    if(!strcmp(TOKEN,"")){
        data->status = 0;
        first_time_authentication(data);
    }

    if(data->status){
        int sport_type =0;

        if(!g_ascii_strcasecmp("Running",data->name)){

            sport_type =1;
        }
        if(!g_ascii_strcasecmp("Cycling",data->name)){

            sport_type =2;
        }
        if(!g_ascii_strcasecmp("Walking",data->name)){

            sport_type =14;
        }
        if(!g_ascii_strcasecmp("Nordic Walking",data->name)){

            sport_type =4;
        }

        DEBUG("Track name %s", data->name);


        gdouble dist = data->distance/1000;
        int avghr = data->heart_rate_avg;
        int maxhr = data->heart_rate_max;

        time_t time_src;
        struct tm time_dest;
        time_src = data->start_time.tv_sec;
        gmtime_r(&time_src, &time_dest);
        gchar* date_string  = g_strdup_printf(
                ("%04d-%02d-%02d"),
                time_dest.tm_year + 1900,
                time_dest.tm_mon + 1,
                time_dest.tm_mday);
        time_src = data->duration.tv_sec;
        gmtime_r(&time_src, &time_dest);

        int hour = time_dest.tm_hour;
        int min = time_dest.tm_min;
        int sec = time_dest.tm_sec;
        DEBUG("COMMENT %s", data->comment);
        if(data->comment == NULL){
            data->comment = "";
        }
        char *post = NULL;
        char *reply = NULL;
	char *gp = oauth_url_escape(gpx	);
	gchar *test_call_uri =  g_strdup_printf( "http://www.heiaheia.com/api/v1/training_logs"\
                                                 "&sport_id=%d&training_log[date]='%s'"\
                                                 "&training_log[duration_h]=%d"\
                                                 "&training_log[duration_m]=%d"\
                                                 "&training_log[duration_s]=%d"\
                                                 "&training_log[avg_hr]=%d"\
                                                 "&training_log[max_hr]=%d"\
                                                 "&training_log[comment]=%s"\
						 "&training_log[gpx]=%s"\ 
						"&training_log[sport_params][0][name]=distance&training_log[sport_params][0][value]=%.2f"\
						 ,sport_type,date_string,hour,min,sec,avghr,maxhr,data->comment,gp,dist);

        char *req_url = oauth_sign_url2(test_call_uri,&post, OA_HMAC, NULL, CONSUMER_KEY,CONSUMER_SECRET, TOKEN, TOKEN_SEC);
        reply = oauth_http_post(req_url,post);
        printf("\nREPLY:'%s'\n",reply);
        if(g_strrstr(reply,"training-log") != NULL){

            hildon_banner_show_information(GTK_WIDGET(data->win),NULL,"Successfully uploaded to HeiaHeia!");
        }
     	if(g_strrstr(reply,"<error>") != NULL){

            hildon_banner_show_information(GTK_WIDGET(data->win),NULL,"Uploading to HeiaHeia failed!");
        }
	g_free(gp);
	g_free(gpx);
        g_free(test_call_uri);
        g_free(date_string);
    }
    DEBUG_END();
}


static int first_time_authentication(AnalyzerView *data){
    DEBUG_BEGIN();
    int status = 0;
    show_information_note(data->win);
    status = get_request_token(data);
    return status;
    DEBUG_END();
}

static int show_authorize_dlg(AnalyzerView *data,gchar *url){
    DEBUG_BEGIN();
    GtkWidget *authorized;
    GtkWidget *hbox;
    int status = 0;
    data->dialog = gtk_dialog_new_with_buttons("Authorization",
                                               GTK_WINDOW(data->win),
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               NULL);
    authorized = gtk_button_new_with_label("Authorized");
    g_signal_connect (G_OBJECT (authorized), "clicked",
                      G_CALLBACK (authorized_clicked), (gpointer)data);
    gtk_widget_set_size_request(authorized,200,200);
    hbox = gtk_hbox_new(TRUE,0);
    gtk_box_pack_start (GTK_BOX (hbox),authorized, FALSE, FALSE, 0);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(data->dialog));
    gtk_container_add (GTK_CONTAINER (area), hbox);

    gtk_window_set_title(GTK_WINDOW(data->dialog), "Authorization");
    gtk_widget_set_size_request(data->dialog,800,100);
    gtk_widget_show_all(data->dialog);
    osso_rpc_t retval;
    system("/usr/bin/dbus-send --type=signal --session /com/nokia/hildon_desktop com.nokia.hildon_desktop.exit_app_view");
    osso_rpc_run(data->osso, "com.nokia.osso_browser", "/com/nokia/osso_browser", "com.nokia.osso_browser","open_new_window",&retval, DBUS_TYPE_STRING,url, DBUS_TYPE_INVALID);
    status = gtk_dialog_run(GTK_DIALOG(data->dialog));
    if(status == GTK_RESPONSE_DELETE_EVENT){
        gtk_widget_destroy (data->dialog);
    }
    DEBUG_END();
    return status;
}

static int parse_reply(const char *reply) {
    DEBUG_BEGIN();
    int rc;
    int ok=1;
    char **rv = NULL;
    TOKEN = NULL;
    TOKEN_SEC = NULL;
    rc = oauth_split_url_parameters(reply, &rv);
    qsort(rv, rc, sizeof(char *), oauth_cmpstringp);
    if( rc==2
        && !strncmp(rv[0],"oauth_token=",11)
        && !strncmp(rv[1],"oauth_token_secret=",18) ) {
        ok=0;
        char *token =strdup(&(rv[0][12]));
        char *secret=strdup(&(rv[1][19]));
        printf("key:    '%s'\nsecret: '%s'\n",token, secret); // XXX
        TOKEN = token;
        TOKEN_SEC = secret;
    }
    if(rv) free(rv);
    DEBUG_END();
    return ok;

}


static int get_request_token(AnalyzerView *data){
    DEBUG_BEGIN();
    char *postarg = NULL;
    char *req_url = NULL;
    char *reply = NULL;
    req_url = oauth_sign_url2(REQUEST_URI, &postarg, OA_HMAC, NULL, CONSUMER_KEY, CONSUMER_SECRET,NULL, NULL);
    reply = oauth_http_post(req_url,postarg);
    if(reply != NULL){
        DEBUG("REPLY: %s\n", reply);
        int status = parse_reply(reply);
        DEBUG("TOKEN: %s\n", TOKEN);
        DEBUG("TOKEN_SEC: %s\n", TOKEN_SEC);

        gchar *url = g_strconcat((gchar*)AUTHORIZE_URI,"/?oauth_token=",(gchar*)TOKEN,NULL);

        DEBUG("\nURL: %s \n", url);
        status =  show_authorize_dlg(data,url);

    }
    return 0;
    DEBUG_END();
}
static int get_access_token(AnalyzerView *data){
    DEBUG_BEGIN();
    char *req_url = NULL;
    char *reply = NULL;
    req_url = oauth_sign_url2(ACCESS_URI, NULL, OA_HMAC, NULL, CONSUMER_KEY, CONSUMER_SECRET, TOKEN, TOKEN_SEC);
    printf("URL: %s\n", req_url);
    reply = oauth_http_get(req_url,NULL);
    printf("REPLY: %s\n", reply);
    parse_reply(reply);
    return 0;
    DEBUG_END();
}
static void authorized_clicked(GtkWidget*button, gpointer user_data){
    DEBUG_BEGIN();
    AnalyzerView *data = (AnalyzerView *)user_data;
    g_return_if_fail(data != NULL);
    get_access_token(data);
    printf("\n ACCESS TOKEN %s\n",TOKEN);
    printf("\n ACCESS SEC %s\n",TOKEN_SEC);
    gconf_helper_set_value_string_simple(data->gconf_helper,TOKEN_KEY,TOKEN);
    gconf_helper_set_value_string_simple(data->gconf_helper,TOKEN_SECRET_KEY,TOKEN_SEC);
    gtk_widget_destroy (data->dialog);
    data->status = 1;
    DEBUG_END();
}

static void
        show_information_note(GtkWidget *parent){
    DEBUG_BEGIN();
    GtkWidget *note;
    gint response;
    note = hildon_note_new_information (NULL,
                                        "Before you can upload activity to HeiaHeia you have to allow eCoach to connect your account.\n"
                                        "eCoach will now open new browser window.\n"
                                        "Once you have authrized eCoach, press \"Authorized\" button on eCoach.");
    response = gtk_dialog_run (GTK_DIALOG (note));
    if (response == GTK_RESPONSE_DELETE_EVENT)
        g_debug ("%s: GTK_RESPONSE_DELETE_EVENT", __FUNCTION__);
    gtk_object_destroy (GTK_OBJECT (note));
    DEBUG_END();
}
