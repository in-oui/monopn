/*	$NetBSD: execute.c,v 1.11 2004/01/27 20:30:30 jsm Exp $	*/

/*
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)execute.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: execute.c,v 1.11 2004/01/27 20:30:30 jsm Exp $");
#endif
#endif /* not lint */

#include "monop.ext"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdint.h>

#define SAVE_MAGIC "MONOPSV1"
#define SAVE_VERSION 1
#define MAX_NAME_LEN 256

typedef	struct stat	STAT;

static char	buf[257];

static bool	new_play;	/* set if move on to new player		*/

static void show_move(void);
static int read_filename(char *, size_t);
static int put_u32(FILE *, uint32_t);
static int get_u32(FILE *, uint32_t *);
static int put_u64(FILE *, uint64_t);
static int get_u64(FILE *, uint64_t *);

/*
 *	This routine executes the given command by index number
 */
void
execute(com_num)
	int com_num; 
{
	new_play = FALSE;	/* new_play is true if fixing	*/
	(*func[com_num])();
	notify();
	force_morg();
	if (new_play)
		next_play();
	else if (num_doub)
		printf("%s rolled doubles.  Goes again\n", cur_p->name);
}

/*
 *	This routine moves a piece around.
 */
void
do_move() 
{
	int r1, r2;
	bool was_jail;

	new_play = was_jail = FALSE;
	printf("roll is %d, %d\n", r1=roll(1, 6), r2=roll(1, 6));
	if (cur_p->loc == JAIL) {
		was_jail++;
		if (!move_jail(r1, r2)) {
			new_play++;
			goto ret;
		}
	}
	else {
		if (r1 == r2 && ++num_doub == 3) {
			printf("That's 3 doubles.  You go to jail\n");
			goto_jail();
			new_play++;
			goto ret;
		}
		move(r1+r2);
	}
	if (r1 != r2 || was_jail)
		new_play++;
ret:
	return;
}

/*
 *	This routine moves a normal move
 */
void
move(rl)
	int rl; 
{
	int old_loc;

	old_loc = cur_p->loc;
	cur_p->loc = (cur_p->loc + rl) % N_SQRS;
	if (cur_p->loc < old_loc && rl > 0) {
		cur_p->money += 200;
		printf("You pass %s and get $200\n", board[0].name);
	}
	show_move();
}

/*
 *	This routine shows the results of a move
 */
static void
show_move() 
{
	SQUARE *sqp;

	sqp = &board[cur_p->loc];
	printf("That puts you on %s\n", sqp->name);
	switch (sqp->type) {
	  case SAFE:
		printf("That is a safe place\n");
		break;
	  case CC:
		cc(); break;
	  case CHANCE:
		chance(); break;
	  case INC_TAX:
		inc_tax(); break;
	  case GOTO_J:
		goto_jail(); break;
	  case LUX_TAX:
		lux_tax(); break;
	  case PRPTY:
	  case RR:
	  case UTIL:
		if (sqp->owner < 0) {
			printf("That would cost $%d\n", sqp->cost);
			if (getyn("Do you want to buy? ") == 0) {
				buy(player, sqp);
				cur_p->money -= sqp->cost;
			}
			else if (num_play > 2)
				bid();
		}
		else if (sqp->owner == player)
			printf("You own it.\n");
		else
			rent(sqp);
	}
}

/*
 *	This routine saves the current game for use at a later date
 */
void
save() 
{
	int i, j, error = 0;
	time_t t;
	struct stat sb;
	FILE *outf;

	printf("Which file do you wish to save it in? ");
	if (!read_filename(buf, sizeof(buf)))
		return;

	/*
	 * check for existing files, and confirm overwrite if needed
	 */

	if (stat(buf, &sb) > -1
	    && getyn("File exists.  Do you wish to overwrite? ") > 0)
		return;

	if ((outf=fopen(buf, "wb")) == NULL) {
		perror(buf);
		return;
	}
	printf("\"%s\" ", buf);
	error |= fwrite(SAVE_MAGIC, 1, 8, outf) != 8;
	error |= put_u32(outf, SAVE_VERSION);
	error |= put_u32(outf, num_play) | put_u32(outf, player) |
	    put_u32(outf, num_doub);
	for (i = 0; i < num_play; i++) {
		size_t len = strlen(play[i].name);
		error |= put_u32(outf, (uint32_t)len);
		error |= fwrite(play[i].name, 1, len, outf) != len;
		error |= put_u32(outf, play[i].num_gojf) |
		    put_u32(outf, play[i].loc) |
		    put_u32(outf, play[i].in_jail) |
		    put_u32(outf, (uint32_t)play[i].money);
	}
	for (i = 0; i <= N_SQRS; i++)
		error |= put_u32(outf, (uint32_t)(int32_t)board[i].owner);
	for (i = 0; i < N_PROP; i++)
		error |= put_u32(outf, prop[i].morg) |
		    put_u32(outf, prop[i].houses);
	for (i = 0; i < N_RR; i++)
		error |= put_u32(outf, rr[i].morg);
	for (i = 0; i < N_UTIL; i++)
		error |= put_u32(outf, util[i].morg);
	for (i = 0; i < 2; i++) {
		error |= put_u32(outf, deck[i].num_cards) |
		    put_u32(outf, deck[i].last_card) |
		    put_u32(outf, deck[i].gojf_used);
		for (j = 0; j < deck[i].num_cards; j++)
			error |= put_u64(outf, deck[i].offsets[j]);
	}
	error |= fflush(outf) == EOF || ferror(outf);
	if (fclose(outf) == EOF)
		error = 1;
	if (error) {
		fprintf(stderr, "could not completely save %s\n", buf);
		return;
	}
	time(&t);
	{
		char datebuf[80];
		if (ctime_r(&t, datebuf) != NULL) {
			datebuf[strcspn(datebuf, "\n")] = '\0';
			printf("[%s]\n", datebuf);
		} else
			putchar('\n');
	}
}

/*
 *	This routine restores an old game from a file
 */
void
restore() 
{
	printf("Which file do you wish to restore from? ");
	if (!read_filename(buf, sizeof(buf)))
		return;
	rest_f(buf);
}

/*
 *	This does the actual restoring.  It returns TRUE if the
 * backup was successful, else false.
 */
int
rest_f(file)
	const char *file; 
{
	int i, j, ok = FALSE;
	FILE *inf = NULL;
	char magic[8], datebuf[80];
	STAT sbuf;
	uint32_t version, saved_players, saved_player, saved_doub, v;
	typedef struct {
		char *name;
		int gojf, loc, jail, money;
	} SAVED_PLAYER;
	SAVED_PLAYER p[MAX_PL] = {{0}};
	short owners[N_SQRS + 1], mortgages[N_PROP + N_RR + N_UTIL];
	short houses[N_PROP];
	int deck_last[2], deck_gojf[2];
	uint64_t *deck_offsets[2] = { NULL, NULL };

	if ((inf=fopen(file, "rb")) == NULL) {
		perror(file);
		return FALSE;
	}
	printf("\"%s\" ", file);
	if (fread(magic, 1, sizeof(magic), inf) != sizeof(magic) ||
	    memcmp(magic, SAVE_MAGIC, sizeof(magic)) != 0 ||
	    get_u32(inf, &version) || version != SAVE_VERSION ||
	    get_u32(inf, &saved_players) || saved_players < 1 ||
	    saved_players > MAX_PL || get_u32(inf, &saved_player) ||
	    saved_player >= saved_players || get_u32(inf, &saved_doub) ||
	    saved_doub > 2)
		goto bad;
	for (i = 0; i < (int)saved_players; i++) {
		uint32_t len;
		if (get_u32(inf, &len) || len == 0 || len > MAX_NAME_LEN)
			goto bad;
		p[i].name = malloc(len + 1);
		if (p[i].name == NULL)
			err(1, NULL);
		if (fread(p[i].name, 1, len, inf) != len)
			goto bad;
		p[i].name[len] = '\0';
		if (get_u32(inf, &v) || v > 2) goto bad;
		p[i].gojf = v;
		if (get_u32(inf, &v) || v > JAIL) goto bad;
		p[i].loc = v;
		if (get_u32(inf, &v) || v > 3) goto bad;
		p[i].jail = v;
		if (get_u32(inf, &v)) goto bad;
		p[i].money = (int32_t)v;
	}
	for (i = 0; i <= N_SQRS; i++) {
		if (get_u32(inf, &v) || (int32_t)v < -1 ||
		    (int32_t)v >= (int)saved_players) goto bad;
		owners[i] = (int32_t)v;
		if (owners[i] >= 0 && board[i].type != PRPTY &&
		    board[i].type != RR && board[i].type != UTIL)
			goto bad;
	}
	for (i = 0; i < N_PROP; i++) {
		if (get_u32(inf, &v) || v > 1) goto bad;
		mortgages[i] = v;
		if (get_u32(inf, &v) || v > 5) goto bad;
		houses[i] = v;
	}
	for (i = N_PROP; i < N_PROP + N_RR + N_UTIL; i++) {
		if (get_u32(inf, &v) || v > 1) goto bad;
		mortgages[i] = v;
	}
	for (i = 0; i < 2; i++) {
		if (get_u32(inf, &v) || v != (uint32_t)deck[i].num_cards)
			goto bad;
		if (get_u32(inf, &v) || v >= (uint32_t)deck[i].num_cards)
			goto bad;
		deck_last[i] = v;
		if (get_u32(inf, &v) || v > 1) goto bad;
		deck_gojf[i] = v;
		deck_offsets[i] = malloc(deck[i].num_cards * sizeof(uint64_t));
		if (deck_offsets[i] == NULL) err(1, NULL);
		for (j = 0; j < deck[i].num_cards; j++) {
			int k, matches = 0;
			if (get_u64(inf, &deck_offsets[i][j])) goto bad;
			for (k = 0; k < deck[i].num_cards; k++)
				if (deck_offsets[i][j] == deck[i].offsets[k])
					matches++;
			if (matches != 1)
				goto bad;
			for (k = 0; k < j; k++)
				if (deck_offsets[i][j] == deck_offsets[i][k])
					goto bad;
		}
	}
	if (getc(inf) != EOF || ferror(inf))
		goto bad;

	/* Only replace the live game after the complete file was validated. */
	if (play != NULL) {
		for (i = 0; i < num_play; i++) {
			OWN *op = play[i].own_list;
			while (op != NULL) {
				OWN *next = op->next;
				free(op);
				op = next;
			}
			free(play[i].name);
		}
		free(play);
	}
	play = calloc(saved_players, sizeof(*play));
	if (play == NULL) err(1, NULL);
	num_play = saved_players;
	player = saved_player;
	num_doub = saved_doub;
	for (i = 0; i < num_play; i++) {
		play[i].name = p[i].name; p[i].name = NULL;
		play[i].num_gojf = p[i].gojf;
		play[i].loc = p[i].loc; play[i].in_jail = p[i].jail;
		play[i].money = p[i].money;
		name_list[i] = play[i].name;
	}
	name_list[num_play] = "done"; name_list[num_play + 1] = NULL;
	for (i = 0; i <= N_SQRS; i++) board[i].owner = owners[i];
	for (i = 0; i < N_PROP; i++) {
		prop[i].morg = mortgages[i]; prop[i].houses = houses[i];
	}
	for (i = 0; i < N_RR; i++) rr[i].morg = mortgages[N_PROP + i];
	for (i = 0; i < N_UTIL; i++)
		util[i].morg = mortgages[N_PROP + N_RR + i];
	for (i = 0; i < N_MON; i++) {
		mon[i].owner = -1;
		mon[i].num_own = 0;
		mon[i].name = mon[i].not_m;
	}
	for (i = 0; i < N_PROP; i++)
		prop[i].monop = FALSE;
	for (i = 0; i < num_play; i++) {
		trading = TRUE;
		for (j = 0; j <= N_SQRS; j++)
			if (board[j].owner == i) add_list(i, &play[i].own_list, j);
		trading = FALSE; set_ownlist(i);
	}
	for (i = 0; i < 2; i++) {
		memcpy(deck[i].offsets, deck_offsets[i],
		    deck[i].num_cards * sizeof(uint64_t));
		deck[i].last_card = deck_last[i]; deck[i].gojf_used = deck_gojf[i];
	}
	cur_p = &play[player];
	fixing = trading = told_em = spec = FALSE;
	ok = TRUE;
	if (fstat(fileno(inf), &sbuf) == 0 &&
	    ctime_r(&sbuf.st_mtime, datebuf) != NULL) {
		datebuf[strcspn(datebuf, "\n")] = '\0';
		printf("[%s]\n", datebuf);
	} else
		putchar('\n');
bad:
	if (!ok)
		fprintf(stderr, "invalid or incompatible saved game: %s\n", file);
	if (inf != NULL) fclose(inf);
	for (i = 0; i < MAX_PL; i++) free(p[i].name);
	free(deck_offsets[0]); free(deck_offsets[1]);
	return ok;
}

static int
read_filename(char *dst, size_t size)
{
	int c;
	size_t len = 0;
	while ((c = getchar()) != '\n' && c != EOF) {
		if (len + 1 < size) dst[len++] = c;
		else while ((c = getchar()) != '\n' && c != EOF) continue;
	}
	dst[len] = '\0';
	if (len == 0) { fprintf(stderr, "No file name given.\n"); return FALSE; }
	return TRUE;
}

static int put_u32(FILE *f, uint32_t v) {
	unsigned char b[4] = { v >> 24, v >> 16, v >> 8, v };
	return fwrite(b, 1, 4, f) != 4;
}
static int get_u32(FILE *f, uint32_t *v) {
	unsigned char b[4];
	if (fread(b, 1, 4, f) != 4) return 1;
	*v = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
	    ((uint32_t)b[2] << 8) | b[3]; return 0;
}
static int put_u64(FILE *f, uint64_t v) {
	return put_u32(f, v >> 32) || put_u32(f, (uint32_t)v);
}
static int get_u64(FILE *f, uint64_t *v) {
	uint32_t hi, lo;
	if (get_u32(f, &hi) || get_u32(f, &lo)) return 1;
	*v = ((uint64_t)hi << 32) | lo; return 0;
}
