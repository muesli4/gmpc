#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug_printf.h"
#include "libmpd.h"


/*************************************************************************************/
MpdObj * mpd_ob_create()
{
	MpdObj * mi = malloc(sizeof(MpdObj));
	if( mi == NULL )
	{
		/* should never happen on linux */
		return NULL;
	}

	
	/* set default values */
	mi->connected = FALSE;
	mi->port = 6600;
	mi->hostname = strdup("localhost");
	mi->password = NULL;
	mi->connection = NULL;
	mi->status = NULL;
	mi->stats = NULL;
	mi->error = 0;
	mi->error_msg = NULL;
	mi->CurrentSong = NULL;
	/* info */
	mi->playlistid = -1;
	mi->songid = -1;
	mi->state = -1;

	/* signals */
	mi->playlist_changed = NULL;
	/* song */
	mi->song_changed = NULL;
	mi->song_changed_signal_pointer = NULL;
	/* status */
	mi->status_changed = NULL;
	mi->status_changed_signal_pointer = NULL;
	/* state */
	mi->state_changed = NULL;
	mi->state_changed_signal_pointer = NULL;

	mi->error_signal = NULL;
	/* connection is locked because where not connected */
	mi->connection_lock = TRUE;
	
	return mi;
}

void mpd_ob_free(MpdObj *mi)
{
	debug_printf(DEBUG_INFO, "mpd_ob_free: destroying MpdObj object\n");
	if(mi->connected)
	{
		/* disconnect */
		mpd_ob_disconnect(mi);
		debug_printf(DEBUG_WARNING, "mpd_ob_free: Connection still running, disconnecting\n");
	}
	if(mi->hostname)
	{
		free(mi->hostname);
	}
	if(mi->password)
	{
		free(mi->password);
	}
	if(mi->error_msg)
	{
		free(mi->error_msg);
	}
	if(mi->connection)
	{
		/* obsolete */
		mpd_closeConnection(mi->connection);
	}
	if(mi->status)
	{
		mpd_freeStatus(mi->status);
	}
	if(mi->stats)
	{
		mpd_freeStats(mi->stats);
	}	
	if(mi->CurrentSong)
	{
		mpd_freeSong(mi->CurrentSong);
	}
	free(mi);
}

int mpd_ob_check_error(MpdObj *mi)
{
	if(mi == NULL)
	{
		return FALSE;
	}

	if(mi->error)
	{
		return TRUE;
	}	

	/* this shouldn't happen, ever */
	if(mi->connection == NULL)
	{
		debug_printf(DEBUG_WARNING, "mpd_ob_check_error: should this happen, mi->connection == NULL?");
		return FALSE;
	}
	if(mi->connection->error)
	{
		/* TODO: map these errors in the future */
		mi->error = mi->connection->error;
		mi->error_msg = strdup(mi->connection->errorStr);	

		debug_printf(DEBUG_ERROR, "mpd_ob_check_error: Following error occured: code: %i msg: %s", mi->error, mi->error_msg);
		mpd_ob_disconnect(mi);
		/* trigger signal for error */
		if(mi->error_signal)
		{
			mi->error_signal(mi, mi->error, mi->error_msg,mi->error_signal_pointer);
		}

		
		return TRUE;
	}

	return FALSE;
}



int mpd_ob_lock_conn(MpdObj *mi)
{
/*	debug_printf(DEBUG_INFO, "mpd_ob_lock_conn: Locking connection\n");
*/
	if(mi->connection_lock)
	{
		debug_printf(DEBUG_WARNING, "mpd_ob_lock_conn: Failed to lock connection, allready locked\n");
		return TRUE;
	}
	mi->connection_lock = TRUE;
	return FALSE;
}

int mpd_ob_unlock_conn(MpdObj *mi)
{
/*	debug_printf(DEBUG_INFO, "mpd_ob_unlock_conn: unlocking connection\n");
 */
	if(!mi->connection_lock)
	{
		debug_printf(DEBUG_WARNING, "mpd_ob_unlock_conn: Failed to unlock connection, allready unlocked\n");
		return FALSE;
	}
	
	mi->connection_lock = FALSE;

	return mpd_ob_check_error(mi);
}

MpdObj * mpd_ob_new_default()
{
	debug_printf(DEBUG_INFO, "mpd_ob_new_default: creating a new mpdInt object\n");
	return mpd_ob_create();
}

MpdObj *mpd_ob_new(char *hostname,  int port, char *password)
{
	MpdObj *mi = mpd_ob_create();
	if(mi == NULL)
	{
		return NULL;
	}
	if(hostname != NULL)
	{
		mpd_ob_set_hostname(mi, hostname);
	}
	if(port != 0)
	{
		mpd_ob_set_port(mi, port);
	}
	if(password != NULL)
	{
		mpd_ob_set_password(mi, password);
	}
	return mi;
}


void mpd_ob_set_hostname(MpdObj *mi, char *hostname)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_set_hostname: mi == NULL\n");
		return;
	}

	if(mi->hostname != NULL)
	{
		free(mi->hostname);
	}
	/* possible location todo some post processing of hostname */
	mi->hostname = strdup(hostname);
}

void mpd_ob_set_password(MpdObj *mi, char *password)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_set_password: mi == NULL\n");
		return;
	}

	if(mi->password != NULL)
	{
		free(mi->password);
	}
	/* possible location todo some post processing of password */
	mi->password = strdup(password);
}

void mpd_ob_set_port(MpdObj *mi, int port)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_set_port: mi == NULL\n");
		return;
	}
	mi->port = port;
}
int mpd_ob_disconnect(MpdObj *mi)
{
	/* set disconnect flag */
	mi->connected = 0;
	/* lock */
	mpd_ob_lock_conn(mi);
	debug_printf(DEBUG_INFO, "mpd_ob_disconnect: disconnecting\n");

	if(mi->connection)
	{
		mpd_closeConnection(mi->connection);
		mi->connection = NULL;
	}
	if(mi->status)
	{
		mpd_freeStatus(mi->status);
		mi->status = NULL;
	}
	if(mi->stats)
	{
		mpd_freeStats(mi->stats);
		mi->stats = NULL;
	}
	if(mi->CurrentSong)
	{
		mpd_freeSong(mi->CurrentSong);
		mi->CurrentSong = NULL;
	}
	mi->playlistid = -1;
	mi->state = -1;
	mi->songid = -1;

	/*don't reset errors */

	return FALSE;
}

int mpd_ob_connect(MpdObj *mi)
{
	if(mi == NULL)
	{
		/* should return some spiffy error here */
		return -1;
	}
	/* reset errors */
	mi->error = 0;
	if(mi->error_msg != NULL)
	{
		free(mi->error_msg);
	}
	mi->error_msg = NULL;
	
	debug_printf(DEBUG_INFO, "mpd_ob_connect: connecting\n");
	
	if(mi->connected)
	{
		/* disconnect */
		mpd_ob_disconnect(mi);
	}

	if(mi->hostname == NULL)
	{
		mpd_ob_set_hostname(mi, "localhost");
	}
	/* make sure this is true */
	if(!mi->connection_lock)
	{
		mpd_ob_lock_conn(mi);
	}
	/* make timeout configurable */
	mi->connection = mpd_newConnection(mi->hostname,mi->port,1);
	if(mi->connection == NULL)
	{
		/* again spiffy error here */
		return -1;
	}
	if(mi->password != NULL && strlen(mi->password) > 0)
	{
		mpd_sendPasswordCommand(mi->connection, mi->password);	
		mpd_finishCommand(mi->connection);
		/* TODO: check if succesfull */
	}	


	/* set connected state */
	mi->connected = TRUE;
	if(mpd_ob_unlock_conn(mi))
	{
		return -1;
	}
	return 0;
}

int mpd_ob_check_connected(MpdObj *mi)
{
	if(mi == NULL)
	{
		return FALSE;
	}
	return mi->connected;
}

int mpd_ob_status_queue_update(MpdObj *mi)
{

	if(!mpd_ob_check_connected(mi))
	{
		debug_printf(DEBUG_INFO,"mpd_ob_status_queue_update: Where not connected\n");
		return TRUE;
	}                                       	
	if(mi->status != NULL)
	{                                  	
		mpd_freeStatus(mi->status);
		mi->status = NULL;
	}
	return FALSE;
}


int mpd_ob_status_update(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		debug_printf(DEBUG_INFO,"mpd_ob_status_update: Where not connected\n");
		return TRUE;
	}
	if(mpd_ob_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_ob_status_set_volume: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	
	if(mi->status != NULL)
	{
		mpd_freeStatus(mi->status);
	}
	mpd_sendStatusCommand(mi->connection);
	mi->status = mpd_getStatus(mi->connection);
	if(mi->status == NULL)
	{
		debug_printf(DEBUG_ERROR,"mpd_ob_status_update: Failed to grab status from mpd\n");
//		return TRUE;
	}
	if(mpd_ob_unlock_conn(mi))
	{
		return TRUE;
	}
	/*
	 * check for changes 
	 */

	if(mi->playlistid != mi->status->playlist)
	{
		/* print debug message */
		debug_printf(DEBUG_INFO, "mpd_ob_status_update: Playlist has changed!");

		/* TODO: Call defined functions */
		if(mi->playlist_changed != NULL)
		{
			mi->playlist_changed(mi, mi->playlistid, mi->status->playlist);
		}


		/* We can't trust the current song anymore. so we remove it */
		/* tags might have been updated */
		if(mi->CurrentSong != NULL)
		{
			mpd_freeSong(mi->CurrentSong);
			mi->CurrentSong = NULL;
		}

		/* save new id */
		mi->playlistid = mi->status->playlist;
	}


	/* playlist change */
	if(mi->state != mi->status->state)
	{
		/* TODO: Call defined functions */
		if(mi->state_changed != NULL)
		{                                                                      		
			mi->state_changed(mi, mi->state,mi->status->state,mi->state_changed_signal_pointer);
		}                                                                                           		

		mi->state = mi->status->state;
	}
	
	if(mi->songid != mi->status->songid)
	{
		/* print debug message */
		debug_printf(DEBUG_INFO, "mpd_ob_status_update: Song has changed %i %i!", mi->songid, mi->status->songid);

		/* TODO: Call defined functions */
		if(mi->song_changed != NULL)
		{                                                                      		
			mi->song_changed(mi, mi->songid,mi->status->songid,mi->song_changed_signal_pointer);
		}
		/* save new songid */
		mi->songid = mi->status->songid;

	}

	
	if(mi->status_changed != NULL)
	{                                                                      		
		mi->status_changed(mi, mi->status_changed_signal_pointer);		
	}

	return FALSE;
}

/* returns TRUE when status is availible, when not availible and connected it tries to grab it */
int mpd_ob_status_check(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("not connected\n");
		return FALSE;
	}
	if(mi->status == NULL)
	{
		/* try to update */
		if(mpd_ob_status_update(mi))
		{
			printf("failed to update status\n");
			return FALSE;
		}
	}
	return TRUE;
}


int mpd_ob_status_get_volume(MpdObj *mi)
{
	if(mi == NULL)
	{
		printf("failed to check mi == NULL\n");
		return -2;
	}
	if(!mpd_ob_status_check(mi))
	{
		printf("Failed to get status\n");
		return MPD_O_FAILED_STATUS;
	}
	return mi->status->volume;
}

int mpd_ob_status_get_total_song_time(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("failed to check mi == NULL\n");
		return -2;
	}
	if(!mpd_ob_status_check(mi))
	{
		printf("Failed to get status\n");
		return MPD_O_FAILED_STATUS;
	}
	return mi->status->totalTime;
}
int mpd_ob_status_get_elapsed_song_time(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("failed to check mi == NULL\n");
		return -2;
	}
	if(!mpd_ob_status_check(mi))
	{
		printf("Failed to get status\n");
		return MPD_O_FAILED_STATUS;
	}
	return mi->status->elapsedTime;
}

int mpd_ob_status_set_volume(MpdObj *mi,int volume)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_status_set_volume: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	/* making sure volume is between 0 and 100 */
	volume = (volume < 0)? 0:(volume>100)? 100:volume;

	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_status_set_volume: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	/* send the command */
	mpd_sendSetvolCommand(mi->connection , volume);
	mpd_finishCommand(mi->connection);
	/* check for errors */

	mpd_ob_unlock_conn(mi);
	/* update status, because we changed it */
	mpd_ob_status_update(mi);
	/* return current volume */
	return mpd_ob_status_get_volume(mi);
}

/*******************************************************************************************
 * PLAYER
 */

int mpd_ob_player_get_state(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_get_state: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(!mpd_ob_status_check(mi))
	{                                        	
		printf("Failed to get status\n");
		return MPD_O_FAILED_STATUS;
	}                                        

	switch(mi->status->state)
	{
		case MPD_STATUS_STATE_PLAY:
			return MPD_OB_PLAYER_PLAY;
		case MPD_STATUS_STATE_STOP:
			return MPD_OB_PLAYER_STOP;
		case MPD_STATUS_STATE_PAUSE:
			return MPD_OB_PLAYER_PAUSE;
		default:
			return MPD_OB_PLAYER_UNKNOWN;
	}
	return MPD_OB_PLAYER_UNKNOWN;
}

int mpd_ob_player_get_current_song_id(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_ob_player_get_state: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(!mpd_ob_status_check(mi))
	{                                        	          	
		debug_printf(DEBUG_ERROR,"Failed to get status\n");
		return MPD_O_FAILED_STATUS;
	}                
	/* check if in valid state */	
	if(mpd_ob_player_get_state(mi) != MPD_OB_PLAYER_PLAY &&
			mpd_ob_player_get_state(mi) != MPD_OB_PLAYER_PAUSE)
	{
		return -1;
	}
	/* just to be sure check */	
	if(!mi->status->playlistLength)
	{
		return -1;
	}
	return mi->status->songid;
}

int mpd_ob_player_get_current_song_pos(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_ob_player_get_state: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(!mpd_ob_status_check(mi))
	{                                        	          	
		debug_printf(DEBUG_ERROR,"Failed to get status\n");
		return MPD_O_FAILED_STATUS;
	}                
	/* check if in valid state */	
	if(mpd_ob_player_get_state(mi) != MPD_OB_PLAYER_PLAY &&
			mpd_ob_player_get_state(mi) != MPD_OB_PLAYER_PAUSE)
	{
		return -1;
	}
	/* just to be sure check */	
	if(!mi->status->playlistLength)
	{
		return -1;
	}
	return mi->status->song;
}




float mpd_ob_status_set_volume_as_float(MpdObj *mi, float fvol)
{
	int volume = mpd_ob_status_set_volume(mi, (int)(fvol*100.0));
	if(volume > -1)
	{
		return (float)volume/100.0;
	}
	return (float)volume;
}



int mpd_ob_player_play_id(MpdObj *mi, int id)
{
	debug_printf(DEBUG_INFO, "mpd_ob_player_play_id: trying to play id: %i\n", id);
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_play: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_player_play: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	mpd_sendPlayIdCommand(mi->connection,id);
	mpd_finishCommand(mi->connection);


	mpd_ob_unlock_conn(mi);
	if(mpd_ob_status_update(mi))
	{
		return MPD_O_FAILED_STATUS;
	}
	return FALSE;
}

int mpd_ob_player_play(MpdObj *mi)
{
	return mpd_ob_player_play_id(mi, -1);
}

int mpd_ob_player_stop(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_play: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_player_play: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	mpd_sendStopCommand(mi->connection);
	mpd_finishCommand(mi->connection);


	mpd_ob_unlock_conn(mi);
	if(mpd_ob_status_update(mi))
	{
		return MPD_O_FAILED_STATUS;
	}
	return FALSE;
}

int mpd_ob_player_next(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_play: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_player_play: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	mpd_sendNextCommand(mi->connection);
	mpd_finishCommand(mi->connection);


	mpd_ob_unlock_conn(mi);
	if(mpd_ob_status_update(mi))
	{
		return MPD_O_FAILED_STATUS;
	}
	return FALSE;
}

int mpd_ob_player_prev(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_play: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_player_play: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	mpd_sendPrevCommand(mi->connection);
	mpd_finishCommand(mi->connection);


	mpd_ob_unlock_conn(mi);
	if(mpd_ob_status_update(mi))
	{
		return MPD_O_FAILED_STATUS;
	}
	return FALSE;
}


int mpd_ob_player_pause(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_play: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_player_play: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	if(mpd_ob_player_get_state(mi) == MPD_OB_PLAYER_PAUSE)
	{
		mpd_sendPauseCommand(mi->connection,0);
		mpd_finishCommand(mi->connection);
	}
	else if (mpd_ob_player_get_state(mi) == MPD_OB_PLAYER_PLAY)
	{
		mpd_sendPauseCommand(mi->connection,1);
		mpd_finishCommand(mi->connection);
	}


	mpd_ob_unlock_conn(mi);
	if(mpd_ob_status_update(mi))
	{
		return MPD_O_FAILED_STATUS;
	}
	return FALSE;
}

int mpd_ob_player_seek(MpdObj *mi, int sec)
{
	int cur_song  = mpd_ob_player_get_current_song_pos(mi);
	if( cur_song < 0 )
	{
		printf("mpd_ob_player_seek: failed to get current song pos\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_play: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_player_play: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	debug_printf(DEBUG_INFO, "mpd_ob_player_seek: seeking in song %i to %i sec\n", cur_song,sec);

	mpd_sendSeekCommand(mi->connection, cur_song,sec);
	mpd_finishCommand(mi->connection);


	mpd_ob_unlock_conn(mi);
	if(mpd_ob_status_update(mi))
	{
		return MPD_O_FAILED_STATUS;
	}
	return FALSE;
}













int mpd_ob_player_get_repeat(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_get_repeat: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(!mpd_ob_status_check(mi))
	{
		printf("mpd_ob_player_get_repeat: Failed grabbing status\n");
		return MPD_O_NOT_CONNECTED;
	}
	return mi->status->repeat;
}

int mpd_ob_player_set_repeat(MpdObj *mi,int repeat)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_set_repeat: not connected\n");	
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_player_set_repeat: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}
	mpd_sendRepeatCommand(mi->connection, repeat);
	mpd_finishCommand(mi->connection);

	mpd_ob_unlock_conn(mi);
	mpd_ob_status_queue_update(mi);
	return FALSE;
}



int mpd_ob_player_get_random(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_get_random: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(!mpd_ob_status_check(mi))
	{
		printf("mpd_ob_player_get_random: Failed grabbing status\n");
		return MPD_O_NOT_CONNECTED;
	}
	return mi->status->random;
}


int mpd_ob_player_set_random(MpdObj *mi,int random)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_set_random: not connected\n");	
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_player_set_random: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}
	mpd_sendRandomCommand(mi->connection, random);
	mpd_finishCommand(mi->connection);

	mpd_ob_unlock_conn(mi);
	mpd_ob_status_queue_update(mi);
	return FALSE;
}

/*******************************************************************************
 * PLAYLIST 
 */
mpd_Song * mpd_ob_playlist_get_song(MpdObj *mi, int songid)
{
	mpd_Song *song = NULL;
	mpd_InfoEntity *ent = NULL;
	if(!mpd_ob_check_connected(mi))
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_playlist_get_song: Not Connected\n");
		return NULL;
	}

	if(mpd_ob_lock_conn(mi))
	{
		return NULL;
	}
	debug_printf(DEBUG_INFO, "mpd_ob_playlist_get_song: Trying to grab song with id: %i\n", songid);
	mpd_sendPlaylistIdCommand(mi->connection, songid);
	ent = mpd_getNextInfoEntity(mi->connection);
	mpd_finishCommand(mi->connection);

	if(mpd_ob_unlock_conn(mi))
	{
		/*TODO free entity. for now this can never happen */
		return NULL;
	}                         	

	if(ent == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_playlist_get_song: Failed to grab song from mpd\n");
		return NULL;
	}

	if(ent->type != MPD_INFO_ENTITY_TYPE_SONG)
	{
		mpd_freeInfoEntity(ent);
		debug_printf(DEBUG_ERROR, "mpd_ob_playlist_get_song: Failed to grab corect song type from mpd\n");
		return NULL;
	}
	song = mpd_songDup(ent->info.song);
	mpd_freeInfoEntity(ent);

	return song;
}


mpd_Song * mpd_ob_playlist_get_current_song(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_playlist_get_current_song: Not Connected\n");
		return NULL;
	}

	if(!mpd_ob_status_check(mi))
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_playlist_get_current_song: Failed to check status\n");
		return NULL; 
	}

	if(mi->CurrentSong != NULL && mi->CurrentSong->id != mi->status->songid)
	{
		debug_printf(DEBUG_WARNING, "mpd_ob_playlist_get_current_song: Current song not up2date, updating\n");
		mpd_freeSong(mi->CurrentSong);
		mi->CurrentSong = NULL;
	}

	if(mi->CurrentSong == NULL)
	{
		/* TODO: this to use the geT_current_song_id function */
		mi->CurrentSong = mpd_ob_playlist_get_song(mi, mpd_ob_player_get_current_song_id(mi));
		if(mi->CurrentSong == NULL)
		{
			debug_printf(DEBUG_ERROR, "mpd_ob_playlist_get_current_song: Failed to grab song\n");
			return NULL;
		}
	}
	return mi->CurrentSong;
}


int mpd_ob_playlist_clear(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_playlist_clear: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_playlist_clear: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	mpd_sendClearCommand(mi->connection);
	mpd_finishCommand(mi->connection);

	/* unlock */
	mpd_ob_unlock_conn(mi);
	return FALSE;
}

int mpd_ob_playlist_shuffle(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_playlist_shuffle: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_playlist_shuffle: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	mpd_sendShuffleCommand(mi->connection);
	mpd_finishCommand(mi->connection);

	/* unlock */
	mpd_ob_unlock_conn(mi);
	return FALSE;

}

void mpd_ob_playlist_save(MpdObj *mi, char *name)
{
	if(name == NULL && strlen(name))
	{
		debug_printf(DEBUG_WARNING, "mpd_ob_playlist_save: name != NULL  and strlen(name) > 0 failed");
		return;
	}
	if(!mpd_ob_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_ob_playlist_save: not connected\n");
		return;
	}
	if(mpd_ob_lock_conn(mi))
	{
		debug_printf(DEBUG_ERROR,"mpd_ob_playlist_save: lock failed\n");
		return;
	}

	mpd_sendSaveCommand(mi->connection,name);
	mpd_finishCommand(mi->connection);

	/* unlock */                                               	
	mpd_ob_unlock_conn(mi);
	return;
}


/* SIGNALS */
void mpd_ob_signal_set_playlist_changed (MpdObj *mi, void *(* playlist_changed)(MpdObj *mi, int old_playlist_id, int new_playlist_id))
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_signal_set_playlist_changed: MpdObj *mi == NULL");
		return;
	}
	mi->playlist_changed = playlist_changed;
}


void mpd_ob_signal_set_song_changed (MpdObj *mi, void *(* song_changed)(MpdObj *mi, int old_song_id, int new_song_id, void *pointer),void *pointer)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_signal_set_song_changed: MpdObj *mi == NULL");
		return;
	}
	mi->song_changed = song_changed;
	mi->song_changed_signal_pointer = pointer;
}


void mpd_ob_signal_set_state_changed (MpdObj *mi, void *(* state_changed)(MpdObj *mi, int old_state, int new_state, void *pointer),void *pointer)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_signal_set_state_changed: MpdObj *mi == NULL");
		return;
	}
	mi->state_changed = state_changed;
	mi->state_changed_signal_pointer = pointer;
}

void mpd_ob_signal_set_status_changed (MpdObj *mi, void *(* status_changed)(MpdObj *mi, void *pointer),void *pointer)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_signal_set_state_changed: MpdObj *mi == NULL");
		return;
	}
	mi->status_changed = status_changed;
	mi->status_changed_signal_pointer = pointer;
}


void mpd_ob_signal_set_error (MpdObj *mi, void *(* error_signal)(MpdObj *mi, int id, char *msg,void *pointer),void *pointer)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_signal_set_error: MpdObj *mi == NULL");
		return;
	}
	mi->error_signal = error_signal;
	mi->error_signal_pointer = pointer;
}
/* more playlist */

MpdData * mpd_ob_playlist_get_artists(MpdObj *mi)
{
	char *string = NULL;
	MpdData *data = NULL;
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_playlist_get_artists: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_playlist_get_artists: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	mpd_sendListCommand(mi->connection,MPD_TABLE_ARTIST,NULL);
	while (( string = mpd_getNextArtist(mi->connection)) != NULL)
	{	
		if(data == NULL)
		{
			data = mpd_ob_new_data_struct();
			data->first = data;
			data->next = NULL;
			data->last = NULL;
		
		}	
		else
		{
			data->next = mpd_ob_new_data_struct();
			data->next->first = data->first;
			data->next->last = data;
			data = data->next;
			data->next = NULL;
		}
		data->type = 1; /* TODO make enum */
		data->value.artist = string;
	}
	mpd_finishCommand(mi->connection);

	/* unlock */
	mpd_ob_unlock_conn(mi);
	if(data == NULL) 
	{
		return NULL;
	}
	return data->first;
}


MpdData *mpd_ob_new_data_struct()
{
	MpdData* data = malloc(sizeof(MpdData));

	data->type = 0;

	data->value.artist = NULL;
	data->value.album = NULL;
	data->value.song = NULL;

	return data;	
}

MpdData * mpd_ob_data_get_next(MpdData *data)
{
	if(data == NULL || data->next != NULL)
	{
		return data->next;
	}
	return data;	
}

int mpd_ob_data_is_last(MpdData *data)
{
	if(data == NULL || data->next == NULL)
	{
		return TRUE;
	}
	return FALSE;	
}

void mpd_ob_free_data_ob(MpdData *data)
{
	if(data == NULL)
	{
		return;
	}	
	while(data->next != NULL)
	{
		data = data->next;
		if(data->last->type == 1)
		{
			free(data->last->value.artist);
		}
		free(data->last);
	}
	if(data->type == 1)
	{
		free(data->value.artist);
	}
	else if (data->type == 2)
	{
		free(data->value.album);
	}
	free(data);
}

/* */

MpdData * mpd_ob_playlist_get_albums(MpdObj *mi,char *artist)
{
	char *string = NULL;
	MpdData *data = NULL;
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_playlist_get_albums: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_playlist_get_albums: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	mpd_sendListCommand(mi->connection,MPD_TABLE_ALBUM,artist);
	while (( string = mpd_getNextAlbum(mi->connection)) != NULL)
	{	
		if(data == NULL)
		{
			data = mpd_ob_new_data_struct();
			data->first = data;
			data->next = NULL;
			data->last = NULL;
		}	
		else
		{
			data->next = mpd_ob_new_data_struct();
			data->next->first = data->first;
			data->next->last = data;
			data = data->next;
			data->next = NULL;
		}
		data->type = 2; /* TODO make enum */
		data->value.album = string;
	}
	mpd_finishCommand(mi->connection);

	/* unlock */
	mpd_ob_unlock_conn(mi);
	if(data == NULL) 
	{
		return NULL;
	}
	return data->first;
}

