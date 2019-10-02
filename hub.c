#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "shared.h"

// Exit Statuses
#define NO_ERROR 0
#define USEAGE_ERROR 1
#define DECK_ACCESS_ERROR 2
#define ERROR_READING_DECK 3
#define START_PROCESS_ERROR 4
#define PLAYER_EXIT 5
#define INVALID_MESSAGE 6
#define SIGINT_EXIT 7

struct Decks {
    char card[17];
    int nextCard;
    struct Decks *next;
};

struct Game {
    struct Decks *currentDeck;
    int emptyDeck;
    int players;
    int alivePlayers;
    char burntCard;
    char move[5];

    //Player A
    FILE *fromA;
    FILE *toA;
    int pointsA;
    char firstCardA;
    char secondCardA;
    int fromChildA;
    int fromParentA;
    
    //Player B
    FILE *fromB;
    FILE *toB;
    int pointsB;
    char firstCardB;
    char secondCardB;
    int fromChildB;
    int fromParentB;
    
    //Player C
    FILE *fromC;
    FILE *toC;
    int pointsC;
    char firstCardC;
    char secondCardC;
    int fromChildC;
    int fromParentC;
    
    //Player D;
    FILE *fromD;
    FILE *toD;
    int pointsD;
    char firstCardD;
    char secondCardD;
    int fromChildD;
    int fromParentD;

};


pid_t pidA = 0;   //Tried a struct but it kept messing up;
pid_t pidB = 0;
pid_t pidC = 0;
pid_t pidD = 0;

/*
 *
 * @params g The game structure 
 * @params 
 * @params 
 */
void reap_children(void) {
    int statusA, statusB, statusC, statusD;

    if (pidA != 0) {
        kill(pidA, SIGKILL);
        waitpid(pidA, &statusA, WUNTRACED);
    }

    if (pidB != 0) {
        kill(pidB, SIGKILL);
        waitpid(pidB, &statusB, WUNTRACED);
    }

    if (pidC != 0) {
        kill(pidC, SIGKILL);
        waitpid(pidC, &statusC, WUNTRACED);
    }

    if (pidD != 0) {
        kill(pidD, SIGKILL);
        waitpid(pidD, &statusD, WUNTRACED);
    }
}

/*
 *
 * @params g The game structure 
 * @params 
 * @params 
 */
void close_pipes(struct Game *g) {

    close(g->fromChildA);
    close(g->fromParentA);
    close(g->fromChildB);
    close(g->fromParentB);
    close(g->fromChildC);
    close(g->fromParentC);
    close(g->fromChildD);
    close(g->fromParentD);
}

/*
 *
 * @params g The game structure 
 * @params 
 * @params 
 */
void exit_hub(struct Game *g, int status) {

    reap_children();
	
    close_pipes(g);

    switch(status) {
        case NO_ERROR:
            exit(NO_ERROR);
        case USEAGE_ERROR:
            fprintf(stderr,
                    "Usage: hub deckfile prog1 prog2 [prog3 [prog4]]\n");
            exit(USEAGE_ERROR);
        case DECK_ACCESS_ERROR:
            fprintf(stderr, "Unable to access deckfile\n");
            exit(DECK_ACCESS_ERROR);
        case ERROR_READING_DECK:
            fprintf(stderr, "Error reading deck\n");
            exit(ERROR_READING_DECK);
        case START_PROCESS_ERROR:
            fprintf(stderr, "Unable to start subprocess\n");
            exit(START_PROCESS_ERROR);
        case PLAYER_EXIT:      
            fprintf(stderr, "Player quit\n");    
            exit(PLAYER_EXIT);              
        case INVALID_MESSAGE:                                          
            fprintf(stderr, "Invalid message received from player\n"); 
            exit(INVALID_MESSAGE);
    }
}

/*
 *
 * @params g The game structure 
 * @params 
 * @params 
 */
void caught_signal(int signal) {
    if (signal == 2) {
        reap_children();
        fprintf(stderr, "SIGINT caught\n");
        exit(SIGINT_EXIT);
    }
    if (signal == 13) {
        return;
    }
}

/*
 *
 * @params g The game structure 
 * @params 
 * @params 
 */
void flush_streams(struct Game *g) {
    fflush(g->toA);
    fflush(g->toB);
    fflush(g->toC);
    fflush(g->toD);
}

/*
 *
 * @params g The game structure 
 * @params 
 * @params 
 */
void check_deck (struct Game *g, char *deck) { 
    int i, ones = 0, twos = 0, threes = 0, fours = 0, fives = 0, sixes = 0,
            sevens = 0, eights = 0;

    if (deck[16] != '\n' || deck[17] != 0) {

    }

    for (i = 0; i < 16; ++i) {
        if (deck[i] < '1' || deck[i] > '8') {
            exit_hub(g, ERROR_READING_DECK);
        }
        switch (deck[i]) {
            case '1':
                ones++;
                break;
            case '2':
                twos++;
                break;
            case '3':
                threes++;
                break;
            case '4':
                fours++;
                break;
            case '5':
                fives++;
                break;
            case '6':
                sixes++;
                break;
            case '7':
                sevens++;
                break;
            case '8':
                eights++;
        }
    }
    if (ones != 5 || twos != 2 || threes != 2 || fours != 2 || fives != 2 || 
            sixes != 1 || sevens != 1 || eights != 1) {
        exit_hub(g, ERROR_READING_DECK);
    }
}

/*
 *
 * @params g The game structure 
 * @params 
 * @params 
 */
void load_deck(char *deckCards, struct Decks *d) {
    int i;

    for (i = 0; i < 16; ++i) {
        d->card[i] = deckCards[i];
    }
    d->card[16] = 'E';
    d->nextCard = 0;
}

/*
 *
 * @params g The game structure 
 * @params 
 * @params 
 */
struct Decks* create_deck(struct Decks *head) {
    struct Decks *d;
    d = malloc(sizeof(*d));
    d->next = head;
    return d;
}

/*
 *
 * @params g The game structure 
 * @params 
 * @params 
 */
void load_deckfile(struct Game *g, char *argv[]) {
    char deckCards[18];
    struct Decks *head, *newDeck, *lastDeck;
    int i = 0;

    FILE *f = fopen(argv[1], "r");
    if (f == NULL) {
        exit_hub(g, DECK_ACCESS_ERROR);
    }

    head = malloc(sizeof(*head));
    head->next = head;
    g->currentDeck = head;
    lastDeck = head;

    while (fgets(deckCards, 18, f)) { 
        check_deck(g, deckCards);
        if (i == 0) {
            load_deck(deckCards, head);
            i++;
            continue;
        }
        newDeck = create_deck(head);
        lastDeck->next = newDeck;
        load_deck(deckCards, newDeck);
        lastDeck = newDeck;
        i++;
    }

    g->emptyDeck = 0;
}

/*
 *
 * @params g The game structure 
 * @params 
 * @params 
 */
void init_game(struct Game *g) {
    
    g->pointsA = '0';
    g->pointsB = '0';
    g->pointsC = '0';
    g->pointsD = '0';

    g->firstCardA = '-';
    g->firstCardB = '-';
    g->firstCardC = '-';
    g->firstCardD = '-';

    g->secondCardA = '-';
    g->secondCardB = '-';
    g->secondCardC = '-';
    g->secondCardD = '-';

    g->fromChildA = 0;
    g->fromParentA = 0;
    g->fromChildB = 0;
    g->fromParentB = 0;
    g->fromChildC = 0;
    g->fromParentC = 0;
    g->fromChildD = 0;
    g->fromParentD = 0;

}


/*
 *
 * @params g The game structure 
 * @params 
 * @params 
 */
void populate_child_info(struct Game *g, FILE *toParent, FILE *toChild, 
        pid_t pid, int fromParent, int fromChild, char player) {
    
    switch (player) {
        case 'A':
            g->fromA = toParent;
            g->toA = toChild;
            pidA = pid;
            g->fromChildA = fromChild;
            g->fromParentA = fromParent;
            break;
        case 'B':
            g->fromB = toParent;
            g->toB = toChild;
            pidB = pid;
            g->fromChildB = fromChild;
            g->fromParentB = fromParent;
            break;
        case 'C':
            g->fromC = toParent;
            g->toC = toChild;
            pidC = pid;
            g->fromChildC = fromChild;
            g->fromParentC = fromParent;
            break;
        case 'D':
            g->fromD = toParent;
            g->toD = toChild;
            pidD = pid;
            g->fromChildD = fromChild;
            g->fromParentD = fromParent;
    }
}

/*
 *
 * @params g The game structure 
 * @params 
 * @params 
 */
void run_child(struct Game *g, char *program, char *playerNo, char* player) {
    pid_t pid;
    int fromParent[2], fromChild[2], devNull;
    FILE *toChild, *toParent;
    
    if (pipe(fromParent) == -1 || pipe(fromChild) == -1) {
        exit_hub(g, START_PROCESS_ERROR);
    }
    
    pid = fork();
    if (pid) {
        close(fromChild[1]);
        close(fromParent[0]);
        toChild = fdopen(fromParent[1], "w");
        toParent = fdopen(fromChild[0], "r"); 
    } else {
        close(fromChild[0]);
        close(fromParent[1]);
        dup2(fromParent[0], STDIN_FILENO);
        dup2(fromChild[1], STDOUT_FILENO);
        devNull = open("/dev/null", O_WRONLY);
        dup2(devNull, STDERR_FILENO);
        close(fromChild[1]);
        close(fromParent[0]);
        execlp(program, program, playerNo, player, NULL);
        exit(START_PROCESS_ERROR);
    }
    
    populate_child_info(g, toParent, toChild, pid, fromParent[1], 
            fromChild[0], player[0]);
}

/* Check that the child player programs loaded by checking that they printed
 * a single '-' character.
 * @params g The game structure 
 */
void check_player_loaded(struct Game *g) {
    FILE *stream;
    char feedback;
    
    for (int i = 0; i < g->players; i++) {
        switch (i) {
            case 0:
                stream = g->fromA;
                break;
            case 1:
                stream = g->fromB;
                break;
            case 2:
                stream = g->fromC;
                break;
            case 3:
                stream = g->fromD;
                break;
        }
        if ((feedback = fgetc(stream)) == EOF) {
            exit_hub(g, START_PROCESS_ERROR);
        }
        if (feedback != '-') {
            exit_hub(g, START_PROCESS_ERROR);
        }
    }
}


/* Initialise the FILE pointers for each player and then load each of the 
 * supplied pointers by forking after creating two pipes for each child
 * program to allow two way communication.
 * @params g The game structure 
 * @params argv The arguments supplied on the command line
 * @params argc The number of arguments supplied on the command line
 */
void load_programs(struct Game *g, char *argv[], int argc) {
    int players;
    char playerNo[5];

    players = argc - 2;
    g->players = players;
    sprintf(playerNo, "%d", players);

    g->fromA = NULL;
    g->fromB = NULL;
    g->fromC = NULL;
    g->fromD = NULL;
    g->toA = NULL;
    g->toB = NULL;
    g->toC = NULL;
    g->toD = NULL;
 
    run_child(g, argv[2], playerNo, "A");
    run_child(g, argv[3], playerNo, "B");

    if (players > 2) {
        run_child(g, argv[4], playerNo, "C");
    }

    if (players > 3) {
        run_child(g, argv[5], playerNo, "D");
    }
}

/* Checks that the number of arguments supplied is within the allowable range
 * then loads the deckfiles (exiting if invalid) then loads the supplied 
 * programs, exiting if invalid. Finally the loaded programs are checked to 
 * make sure they behave as expected (print a '-') upon startup.
 * @params g The game structure 
 * @params argv The arguments supplied on the command line
 * @params argc The number of arguments supplied on the command line
 */
void parse_args(struct Game *g, char *argv[], int argc) {

    if (argc < 4 || argc > 6) {
        exit_hub(g, USEAGE_ERROR);
    }

    load_deckfile(g, argv);
    load_programs(g, argv, argc);
    check_player_loaded(g);
}

/* Gets a new cards from the deck. If the card is 'E', which is a dummy
 * card singling the end of the deck, the deck is set as empty.
 * @params g The game structure 
 * @return the new card selected from the deck
 */
char new_card(struct Game *g) {
    struct Decks *deck;
    char card;
    int newCard;

    deck = g->currentDeck;
    newCard = deck->nextCard++;

    card = deck->card[newCard];

    if (card == 'E') {
        g->emptyDeck = 1;
    }
    return card;
}

/* Initiate a new round by distributing new cards to the playing players
 * using the newround message and setting the number of alive players to
 * the number of players. Also makes sure to set aside the first card in
 * the deck
 * @params g The game structure 
 */
void new_round(struct Game *g) {

    g->burntCard = new_card(g);
    
    g->firstCardA = new_card(g);
    g->firstCardB = new_card(g);
    g->firstCardC = '-';
    g->firstCardD = '-';
    g->secondCardA = '-';
    g->secondCardB = '-';
    g->secondCardC = '-';
    g->secondCardD = '-';
    g->alivePlayers = g->players;
   
    fprintf(g->toA, "newround %c\n", g->firstCardA);
    fprintf(g->toB, "newround %c\n", g->firstCardB);

    if (g->players > 2) {
        g->firstCardC = new_card(g);
        fprintf(g->toC, "newround %c\n", g->firstCardC);
    }
    if (g->players > 3) {
        g->firstCardD = new_card(g);
        fprintf(g->toD, "newround %c\n", g->firstCardD);
    }
    flush_streams(g);
}

/* Checks to make sure the card discarded supplied by the player whose turn it
 * is is actually held by the player.
 * @params g The game structure 
 * @params player The player whose hand to check
 * @params discard The card the player discarded to compare against hand
 */
void check_card_held(struct Game *g, int player, char discard) {
    char firstCard, secondCard;

    switch (player) {
        case 0:
            firstCard = g->firstCardA;
            secondCard = g->secondCardA;
            if (discard == firstCard) {
                g->firstCardA = secondCard;
            }
            g->secondCardA = '-';
            break;
        case 1:         
            firstCard = g->firstCardB;
            secondCard = g->secondCardB;
            if (discard == firstCard) {
                g->firstCardB = secondCard;
            }
            g->secondCardB = '-';
            break;
        case 2:
            firstCard = g->firstCardC;
            secondCard = g->secondCardC;
            if (discard == firstCard) {
                g->firstCardC = secondCard;
            }
            g->secondCardC = '-';
            break;
        case 3:
            firstCard = g->firstCardD;
            secondCard = g->secondCardD;
            if (discard == firstCard) {
                g->firstCardD = secondCard;
            }
            g->secondCardD = '-';
            break;
    }
    if (discard != firstCard && discard != secondCard) {
        exit_hub(g, INVALID_MESSAGE);
    }
}

/* Get the current hand (firstCard) of the specified player, when it is not
 * their turn. 
 * @params g The game structure 
 * @params player
 * @return The firstCard (hand) of the relevant player. Blank if specified 
 * player is not A,B,C or D- which should not occur.
 */
char get_hand(struct Game *g, char player) {
    switch (player) {
        case 'A':
            return (g->firstCardA);
        case 'B':
            return (g->firstCardB);
        case 'C':
            return (g->firstCardC);
        case 'D':
            return (g->firstCardD);
    }
    return '-';
}


char get_player_output(struct Game *g, FILE *source) {
    char output;

    if ((output = fgetc(source)) == EOF) {
        exit_hub(g, PLAYER_EXIT);
    }
    return output;
}


/* Gets a message (a move) from the specified player
 * @params g The game structure 
 * @params player The player to get the move from
 */
void get_move(struct Game *g, int player) {
    char move[5];

    switch (player) {
        case 0:
            if (fgets(move, 5, g->fromA) == NULL) {
                exit_hub(g, PLAYER_EXIT);
            }
            move[3] = 'A';
            break;
        case 1:
            if (fgets(move, 5, g->fromB) == NULL) {
                exit_hub(g, PLAYER_EXIT);
            }
            move[3] = 'B';
            break;
        case 2:
            if (fgets(move, 5, g->fromC) == NULL) {
                exit_hub(g, PLAYER_EXIT);
            }
            move[3] = 'C';
            break;
        case 3:
            if (fgets(move, 5, g->fromD) == NULL) {
                exit_hub(g, PLAYER_EXIT);
            }
            move[3] = 'D';
            break;
    }

    strncpy(g->move, move, 5);
}

/* Sends a replace message to the specified player, their current hand
 * is changed to the card supplied
 * @params g The game structure 
 * @params target the player targeted by the replace message
 * @params card the card to replace the targets hand with
 */
void replace_hand(struct Game *g, char target, char card) {

    switch (target) {
        case 'A':
            g->firstCardA = card;
            fprintf(g->toA, "replace %c\n", card);
            break;
        case 'B':
            g->firstCardB = card;
            fprintf(g->toB, "replace %c\n", card);
            break;
        case 'C':
            g->firstCardC = card;
            fprintf(g->toC, "replace %c\n", card);
            break;
        case 'D':
            g->firstCardD = card;
            fprintf(g->toD, "replace %c\n", card);
            break;
    }
    flush_streams(g);
}

/* Marks a player as out by setting their cards to '!' and decrements the
 * alive player count by one. If the player supplied is not A, B, C or D
 * no action is taken.
 * @params g The game structure
 * @params out The player to mark as out
 */
void set_player_out(struct Game *g, char out) {
    switch (out) {
        case 'A':
            g->firstCardA = '!';
            g->secondCardA = '!';
            break;
        case 'B':
            g->firstCardB = '!';
            g->secondCardB = '!';
            break;
        case 'C':
            g->firstCardC = '!';
            g->secondCardC = '!';
            break;
        case 'D':
            g->firstCardD = '!';
            g->secondCardD = '!';
            break;
        default:
            return;

    }
    g->alivePlayers--;
}

/* Sends a thishappened message to all players reporting what happened as a
 * result of the last players turn. '-' is used if a param is N/A
 * @params g The game structure 
 * @params source The label of the player whose turn it was
 * @params discard The card discarded by source player
 * @params target The player targeted by the source player. 
 * @params guess The card guessed by the source player. 
 * @params dropper The player who dropped a card as a result of the turn
 * @params dropped The card dropped (by the dropper) as a result of the turn
 * @params out The player who became out due to the turn
 */
void this_happened(struct Game *g, char source, char discard, char target, 
        char guess, char dropper, char dropped, char out) {
    char message[23];

    sprintf(message, "thishappened %c%c%c%c/%c%c%c\n", source, discard, 
            target, guess, dropper, dropped, out);

    fprintf(g->toA, "%s", message);
    fprintf(g->toB, "%s", message);
    if (g->players > 2) {
        fprintf(g->toC, "%s", message);
    }
    if (g->players > 3) {
        fprintf(g->toD, "%s", message);
    }

    flush_streams(g);

    fprintf(stdout, "Player %c discarded %c", source, discard);

    if (target != '-') {
        fprintf(stdout, " aimed at %c", target);
    }

    if (guess != '-') {
        fprintf(stdout, " guessing %c", guess);
    } 
    fprintf(stdout, ".");

    if (dropped != '-') {
        fprintf(stdout, " This forced %c to discard %c.", dropper, dropped);
    }

    if (out != '-') {
        fprintf(stdout, " %c was out.", out);
    }

    fprintf(stdout, "\n");
    fflush(stdout);

    set_player_out(g, out); 
}

/* Forces a player to discard their hand and take a new card by replacing
 * their current card with a newly drawn card (from deck or the burntCard)
 * If the discard causes a player to drop an eight, they are also set as out
 * A thishappened message is then sent to all players. 
 * @params g The game structure 
 * @params source the player who forced the discard
 * @params target the player forced to discard their hand
 */
void discard_hand(struct Game *g, char source, char target) {
    char hand = get_hand(g, target), discard = '5', nill = '-', out = '-';
    char newCard;

    if (target == '-') {
        this_happened(g, source, discard, nill, nill, nill, nill, nill);
        return;
    }

    if (hand == '8') {
        out = target;
    } else {
        newCard = new_card(g);
        if (newCard == 'E') {
            newCard = g->burntCard;
            g->emptyDeck = 1;
        }
    }
	
    replace_hand(g, target, newCard);
	
    this_happened(g, source, discard, target, nill, target, hand, out);
    
}

/* Swaps the hands of the two supplied players and sends a thishappened
 * message reporting the swap (but not contents of the hands) to all players.
 * @params g The game structure 
 * @params target the player forced to swap
 * @params source the player who initiated the swap
 */
void swap_hands(struct Game *g, char target, char source) {
    char discard = '6', nill = '-';
    char targetHand = get_hand(g, target);
    char sourceHand = get_hand(g, source);

    if (target != '-') {
        replace_hand(g, target, sourceHand);
        replace_hand(g, source, targetHand);
    }

    this_happened(g, source, discard, target, nill, nill, nill, nill);
}

/* Given a supplied target and source player compares their hands and 
 * sets the lower hand as out. If hands are equal no one is set as out.
 * This is called when a player plays card '3'.
 * The result is then sent to the players with a thishappened message.
 * @params g The game structure 
 * @params target the player targeted for comparison by the source player
 * @params source the player who caused this action by player the '3' card
 */
void get_out(struct Game *g, char target, char source) {
    char out = '-', nill = '-', hand = 0, discard = '3';

    if (get_hand(g, source) < get_hand(g, target)) {
        out = source;
    } else if (get_hand(g, source) > get_hand(g, target)) {
        out = target;
    }
    if (out != '-') {
        hand = get_hand(g, out);
        this_happened(g, source, discard, target, nill, out, hand, out);
    } else {
        this_happened(g, source, discard, target, nill, nill, nill, nill);
    }
}


/* Receives a move from a player and checks to see if it was valid.
 * If valid the results of this move are calculated and printed to stdout
 * and sent to all players as a thishappened message. If a player becomes out
 * then they are set to be out.
 * @params g The game structure 
 * @params player The player who made the move
 */
void process_move(struct Game *g, int player) {
    get_move(g, player);  

    char source = g->move[3], discard = g->move[0], target = g->move[1],
            guess = g->move[2], nill = '-';

    check_card_held(g, player, discard);
    check_valid_move(source, discard, target, guess, g->players);

    switch (discard) {
        case '1':
            if (guess == get_hand(g, target)) {
                this_happened(g, source, discard, target, guess, target, 
                        guess, target);
            } else {
                this_happened(g, source, discard, target, guess, nill, nill, 
                        nill);
            }
            break;
        case '3':
            get_out(g, target, source);
            break;
        case '5':
            discard_hand(g, source, target);
            break;            
        case '6':
            swap_hands(g, target, source);
            break;
        default:
            this_happened(g, source, discard, nill, nill, nill, nill, nill);
    }

}

/* Finds the highest value from a supplied list of four characters and 
 * returns this value.
 * @params statusA Card player A was holding, or '!'/'-' if dead/not playing
 * @params statusB Card player B was holding, or '!'/'-' if dead/not playing
 * @params statusC Card player C was holding, or '!'/'-' if dead/not playing
 * @params statusD Card player D was holding, or '!'/'-' if dead/not playing
 * @return returns the highest supplied character
 */
char find_highest(char statusA, char statusB, char statusC, char statusD) {
    char i = '9';

    while (i > 0) {
        if (i == statusA || i == statusB || i == statusC || i == statusD) {
            return i;
        }
        i--;
    }
    return 0;
}

/* Finds the player with the highest card - eliminated players are marked 
 * using '!' which is lower than the numbered cards alive players have.
 * Non playing players are initialised with a '-', which is also lower than
 * any numbered cards.
 * If a player has this card their points are increased and they are 
 * announced as one of the winners.
 * @params g The game structure 
 */
void print_winner(struct Game *g, char statusA, char statusB, char statusC, 
        char statusD) {
    char highest = find_highest(statusA, statusB, statusC, statusD);

    fprintf(stdout, "Round winner(s) holding %c:", highest);

    if (highest == statusA) {
        fprintf(stdout, " A");
        g->pointsA++;
    }
    if (highest == statusB) {
        fprintf(stdout, " B");
        g->pointsB++;
    }
    if (highest == statusC) {
        fprintf(stdout, " C");
        g->pointsC++;
    }
    if (highest == statusD) {
        fprintf(stdout, " D");
        g->pointsD++;
    }
    fprintf(stdout, "\n");
    fflush(stdout);   
}

/* Sends the scores of each player to all of the players.
 * @params g The game structure 
 */
void send_scores(struct Game *g) {
    char scores[16];
    int players = g->players;
 
    if (players == 2) {
        sprintf(scores, "scores %c %c", g->pointsA, g->pointsB);
    }
    if (g->players == 3) {
        sprintf(scores, "scores %c %c %c", g->pointsA, g->pointsB, g->pointsC);
    }
    if (g->players == 4) {
        sprintf(scores, "scores %c %c %c %c", g->pointsA, g->pointsB,
                g->pointsC, g->pointsD);
    } 

    fprintf(g->toA, "%s\n", scores);
    fprintf(g->toB, "%s\n", scores);
    if (g->players > 2) {
        fprintf(g->toC, "%s\n", scores);
    }
    if (g->players > 3) {
        fprintf(g->toD, "%s\n", scores);
    }

    flush_streams(g);
}

/* Gets the next deck ready at the end of a round, prints the winner
 * of the last round and sends the scores to each player. 
 * @params g The game structure 
  */
void end_of_round(struct Game *g) {
    struct Decks *deck;
    char statusA, statusB, statusC, statusD;

    deck = g->currentDeck;
    deck->nextCard = 0;

    deck = (deck->next);
    deck->nextCard = 0;

    g->currentDeck = deck;
    g->emptyDeck = 0;

    statusA = g->firstCardA;
    statusB = g->firstCardB;
    statusC = g->firstCardC;
    statusD = g->firstCardD;

    print_winner(g, statusA, statusB, statusC, statusD);

    send_scores(g);

}

/* Checks if the player supplied is dead. If the player is dead a 1 is 
 * returned, otherwise a 0 is returned.
 * @params g The game structure 
 * @return A 1 if the player is dead, a 0 if alive.
 */
int player_dead(struct Game *g, int player) {
    switch (player) {
        case 0:
            if (g->firstCardA == '!' || g->secondCardA == '!') {
                return 1;
            }
            return 0;
        case 1:
            if (g->firstCardB == '!' || g->secondCardB == '!') {
                return 1;
            }
            return 0;
        case 2:
            if (g->firstCardC == '!' || g->secondCardC == '!') {
                return 1;
            }
            return 0;
        case 3:
            if (g->firstCardD == '!' || g->secondCardD == '!') {
                return 1;
            }
            return 0;
    }
    return 0;
}





/* A turn of the game is repeatedly played until there is only one alive 
 * player left or the deck runs out of cards. Alive players are given a new
 * card in turn and their move is processed.Once a round is over the winner(s)
 * are printed and housekeeping is performed and the next deck readied. 
 * @params g The game structure 
 */
void play_round(struct Game *g) {
    char card;
    int player;

    while (g->alivePlayers > 1) { 
        for (player = 0; player < g->players; ++player) {
            if (player_dead(g, player)) {
                continue;
            }
            if (!(g->emptyDeck)) {
                card = new_card(g);
            } else {
                end_of_round(g);
                return;
            }
            if (card != 'E') {
                switch (player) {
                    case 0:
                        fprintf(g->toA, "yourturn %c\n", card);
                        g->secondCardA = card;
                        break;
                    case 1: 
                        fprintf(g->toB, "yourturn %c\n", card);
                        g->secondCardB = card;
                        break;
                    case 2:
                        fprintf(g->toC, "yourturn %c\n", card);
                        g->secondCardC = card;
                        break;
                    case 3:
                        fprintf(g->toD, "yourturn %c\n", card);
                        g->secondCardD = card;
                        break;
                }
                flush_streams(g);
                process_move(g, player);
            } else {
                g->emptyDeck = 1;
                end_of_round(g);
                return;
            }
        }
    }
    end_of_round(g);
}

/* Starts a new round and then plays a round while the points of each 
 * player are less than four. Once the game is over the winner(s) are printed
 * and a gameover message is sent to all of the players.
 * @params g The game structure
 */
void play_game(struct Game *g) {
    while (g->pointsA < '4' && g->pointsB < '4' && g->pointsC < '4' && 
            g->pointsD < '4') {
        new_round(g);
        play_round(g);
    }
    fprintf(stdout, "Winner(s):");

    if (g->pointsA == '4') {
        fprintf(stdout, " A");
    }
    if (g->pointsB == '4') {
        fprintf(stdout, " B");
    }
    if (g->pointsC == '4') {
        fprintf(stdout, " C");
    }
    if (g->pointsD == '4') {
        fprintf(stdout, " D");
    }
    fprintf(stdout, "\n");
    fflush(stdout);

    fprintf(g->toA, "gameover\n");
    fprintf(g->toB, "gameover\n");

    if (g->players > 2) {
        fprintf(g->toC, "gameover\n");
    }

    if (g->players > 3) {
        fprintf(g->toD, "gameover\n");
    }

    flush_streams(g);
}


int main(int argc, char *argv[]) {
    struct Game *g = NULL;
    struct sigaction sa;

    sa.sa_handler = caught_signal;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, 0);
    sigaction(SIGPIPE, &sa, 0);

    g = malloc(sizeof(*g));

    parse_args(g, argv, argc);

    init_game(g);

    play_game(g);    

    exit_hub(g, NO_ERROR);
}



