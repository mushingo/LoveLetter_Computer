#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shared.h"

// Exit Status
#define NO_ERROR 0
#define USEAGE_ERROR 1
#define INVALID_PLAYERS 2 
#define INVALID_ID 3
#define HUB_LOSS 4
#define INVALID_HUB 5


#define MIN_PLAYERS 2
#define MAX_PLAYERS 4
#define FIRST_LABEL 'A'

struct Player {
    int players;
    char label;
    char firstCard;
    char secondCard;
    char cardsPlayedA[9];
    char cardsPlayedB[9];
    char cardsPlayedC[9];
    char cardsPlayedD[9];
    char statusA;
    char statusB;
    char statusC;
    char statusD;
    char playCard;
    char targetPlayer;
    char guessedCard;
};

struct CardsPlayed {
    int ones;
    int twos;
    int threes;
    int fours;
    int fives;
    int sixes;
    int sevens;
    int eights;
};

/*
 * Exit player with appropriate code after freeing memory
 * @param p The Player structure
 * @param status The exit status to use.
 */
void exit_player(struct Player *p, int status) {

    free(p);

    switch(status) {
        case NO_ERROR:
            exit(NO_ERROR);
        case USEAGE_ERROR:
            fprintf(stderr, "Usage: player number_of_players myid\n");
            exit(USEAGE_ERROR);
        case INVALID_PLAYERS:
            fprintf(stderr, "Invalid player count\n");
            exit(INVALID_PLAYERS);
        case INVALID_ID:
            fprintf(stderr, "Invalid player ID\n");
            exit(INVALID_ID);
        case HUB_LOSS:
            fprintf(stderr, "Unexpected loss of hub\n");
            exit(HUB_LOSS);
        case INVALID_HUB:
            fprintf(stderr, "Bad message from hub\n");
            exit(INVALID_HUB);
    }

}

/* Checks to see if a seven should be discarded due to holding a six or a five
 * if so the seven is chosen and the remaining card is set as the firstCard and
 * the second card is set to be bank. A 1 is then returned to indicate that a 
 * seven was discarded. Otherwise a 0 is returned.
 * the remaining card is set as the firstCard and the
 * second card is set to be bank.
 * @params The player structure
 * @return Returns 1 if a seven is discarded and a 0 otherwise.
 */
int discard_seven(struct Player *p) {
    char firstCard = p->firstCard;
    char secondCard = p->secondCard;
    
    if ((firstCard == '7') && (secondCard == '5' || secondCard == '6')) {
        p->playCard = '7';
        p->firstCard = secondCard;
        p->secondCard = '-';
        return 1;
    }

    if ((secondCard == '7') && (firstCard == '5' || firstCard == '6')) {
        p->playCard = '7';
        p->secondCard = '-';
        return 1;
    }
    return 0;
}

/* Looks at two cards held by player and discards the lowest of these two.
 * After discarding card, the remaining card is set as the firstCard and the
 * second card is set to be bank.
 * @params The player structure
 * @return Returns 1 if a lower (or equal) card can be selected. Should always
 * return 1.
 */
int discard_lowest(struct Player *p) {
    char firstCard = p->firstCard;
    char secondCard = p->secondCard;

    if (firstCard < secondCard) {
        p->playCard = firstCard;
        p->firstCard = secondCard;
        p->secondCard = '-';
        return 1;
    }

    if (secondCard <= firstCard) {
        p->playCard = secondCard;
        p->secondCard = '-';
        return 1;
    }
    return 0;

}

/* Takes a player label and returns a number corresponding to the next player
 * in order A (1)  > B (2) > C (3) > D (4) > A (1).
 * @params label The label of the starting player
 * @return The number corresponding to the next player
 */
int next_player(char label) {
    switch (label) {
        case 'A':
            return 1;
        case 'B':
            return 2;
        case 'C':
            return 3;
        case 'D':
            return 0;
    }
    return 0;
}
    
/* Selects the next non protected and not out player to be targeted. If the
 * noSelf paramater is 0, the target can be this player, otherwise only other
 * players are targeted. If there are no players available to target, a blank
 * target ('-') is selected.
 * @params The player structure
 * @params Indicates if this player should be targeted, 1 if so, 0 if not
 */
void target_player(struct Player *p, int noSelf) {
    char label = p->label;
    int i = 0, stopSelfTarget;
    char status[4] = {p->statusA, p->statusB, p->statusC, p->statusD};

    if (noSelf) {
        stopSelfTarget = next_player(label);
        if (stopSelfTarget == 0) {
            stopSelfTarget = 4;
        } else {
            stopSelfTarget--;
        }
        status[stopSelfTarget] = 'S';
    }
 
    while ((status[(next_player(label))] != ' ') && i < 6) {
        label++;
        i++;
        if (label == 'E') {
            label = 'A';
        }
    }
    
    label++;
    if (label == 'E') {
        label = 'A';
    }

    p->targetPlayer = label;
    if (i >= 6) {
        p->targetPlayer = '-';
    }
}

/* Sets the supplied player status to indicate that the player is protected
 * @params p The player structure
 * @params label The player to remove protection from
 */
void protect_player(struct Player *p, char label) {

    switch (label) {
        case 'A':
            p->statusA = '*';
            break;
        case 'B':
            p->statusB = '*';
            break;
        case 'C':
            p->statusC = '*';
            break;
        case 'D':
            p->statusD = '*';
            break;
    }
    return;
}

/* If the supplied player is protected, the protection is removed from the 
 * the player status.
 * @params p The player structure
 * @params label The player to remove protection from
 */
void remove_protection(struct Player *p, char label) {
  
    switch (label) {
        case 'A':
            if (p->statusA == '*') {
                p->statusA = ' ';
            }
            break;
        case 'B':
            if (p->statusB == '*') {
                p->statusB = ' ';
            }
            break;
        case 'C':
            if (p->statusC == '*') {
                p->statusC = ' ';
            }
            break;
        case 'D':
            if (p->statusD == '*') {
                p->statusD = ' ';
            }
            break;
    }
    return;
}

/* A supplied card is used to decrement the number of cards of a certain type 
 * remaining in the game as recorded by the card count structure. 
 * @params cardPlayed the card type to decrement
 * @params p The player structure
 * @params cp The card count structure
 */
void count_card(char cardPlayed, struct Player *p, struct CardsPlayed *cp) {

    switch (cardPlayed) {
        case 0:
            return;
        case '1':
            cp->ones--;
            return;
        case '2':
            cp->twos--;
            return;
        case '3':
            cp->threes--;
            return;
        case '4':
            cp->fours--;
            return;
        case '5':
            cp->fives--;
            return;
        case '6':
            cp->sixes--;
            return;
        case '7':
            cp->sevens--;
            return;
        case '8':
            cp->eights--;
            return;
    }
}

/* A card to guess is picked in the case that a 1 is played by the player.
 * How many cards of each type played thus far is calculated, and the highest 
 * card not yet played is then picked.
 * @params p The player structure
 */
void pick_card(struct Player *p) {
    int i;
    struct CardsPlayed *cp = NULL;

    if (p->targetPlayer == '-') {
        p->guessedCard = '-';
        return;
    }
    
    cp = malloc(sizeof(*cp));

    cp->ones = 5;
    cp->twos = 2;
    cp->threes = 2;
    cp->fours = 2;
    cp->fives = 2;
    cp->sixes = 1;
    cp->sevens = 1;
    cp->eights = 1;

    for (i = 0; i < 9; ++i) {
        count_card(p->cardsPlayedA[i], p, cp);
        count_card(p->cardsPlayedB[i], p, cp);
        count_card(p->cardsPlayedC[i], p, cp);
        count_card(p->cardsPlayedD[i], p, cp);
    }
    
    if (cp->eights) {
        p->guessedCard = '8';
    } else if (cp->sevens) {
        p->guessedCard = '7';
    } else if (cp->sixes) {
        p->guessedCard = '6';
    } else if (cp->fives) {
        p->guessedCard = '5';
    } else if (cp->fours) {
        p->guessedCard = '4';
    } else if (cp->threes) {
        p->guessedCard = '3';
    } else if (cp->twos) {
        p->guessedCard = '2';
    } else if (cp->ones) {
        p->guessedCard = '1';
    }
}


/* The card chosen to be played is processed in order to apply 
 * whatever effect this card should have.
 * @params p The player structure
 */
void process_card_effect(struct Player *p) {

    char playedCard = p->playCard;

    switch (playedCard) {
        case '6':
            target_player(p, 1);
            break;
        case '5':
            target_player(p, 0);
            break;
        case '4':
            protect_player(p, p->label);
            break;
        case '3':
            target_player(p, 1);
            break;
        case '1':
            target_player(p, 1);
            pick_card(p);
            break;
        default:
            break;

    }
}

/* Sets move to to hub blank, and then decides if a seven should be discarded
 * or if the lowest card should be discarded. Next the effect of discarding 
 * the chosen card is calculated and the move to be sent to the hub is
 * populated.
 * @params p The player structure
 */
void process_move(struct Player *p) {

    p->playCard = '-';
    p->targetPlayer = '-';
    p->guessedCard = '-';
    
    if (discard_seven(p) || discard_lowest(p)) {
        process_card_effect(p);
        return;
    }
}

/* Setup a new round by giving players blank cards if playing and setting 
 * their cards to zero otherwise. Statuses are also set to blank if playing
 * and zero if not. 
 * @params p The player structure
 */
void init_round(struct Player *p) { 
    char blankCards[] = {'-', '-', '-', '-', '-', '-', '-', '-', '-'};
    char notPlayingCards[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

    strncpy(p->cardsPlayedA, blankCards, 9);
    strncpy(p->cardsPlayedB, blankCards, 9);
    strncpy(p->cardsPlayedC, blankCards, 9);
    strncpy(p->cardsPlayedD, blankCards, 9);
    p->statusA = ' ';
    p->statusB = ' ';
    p->statusC = ' ';
    p->statusD = ' ';
    p->firstCard = '-';
    p->secondCard = '-';

    switch (p->players) {
        case 2:
            strncpy(p->cardsPlayedC, notPlayingCards, 9);
            p->statusC = 0;
        case 3:
            strncpy(p->cardsPlayedD, notPlayingCards, 9);
            p->statusD = 0;
    }
}

/* Checks and processes the commands supplied upon program startup
 * Exits with appropriate message if the values supplied are not valid
 * @params p The player structure
 * @params argv An array of strings with each string being each argument
 */
void parse_args(struct Player *p, char *argv[]) {
    char *nextInput;
    long playerNo;
    
    playerNo = strtol(argv[1], &nextInput, 10);
    if (*nextInput != 0 || nextInput == argv[1] || playerNo < 2 || 
           playerNo > 4) {
        exit_player(p, INVALID_PLAYERS);
    }
    p->players = (int) playerNo;

    if (argv[2][1] != 0 || argv[2][0] < FIRST_LABEL ||
            argv[2][0] >= FIRST_LABEL + playerNo) {
        exit_player(p, INVALID_ID);    
    }

    p->label = argv[2][0];
    init_round(p);    

}

/* Prints the status of each player playing (out or not) as well as the cards
 * they have discarded thus far.
 * @params p The player structure
 * @params The player to print the status and discarded cards for
 */
void print_player_status(struct Player *p, int player) {
    char card; 
    int j = 0;
    
    switch (player) {
        case 0:
            fprintf(stderr, "A%c:", p->statusA);
            card = p->cardsPlayedA[j];
            while (card != '-') {
                fprintf(stderr, "%c", card);
                card = p->cardsPlayedA[++j];
            }
            break;
        case 1:
            fprintf(stderr, "B%c:", p->statusB);
            card = p->cardsPlayedB[j];
            while (card != '-') {
                fprintf(stderr, "%c", card);
                card = p->cardsPlayedB[++j];
            }          
            break;
        case 2:
            fprintf(stderr, "C%c:", p->statusC);
            card = p->cardsPlayedC[j];
            while (card != '-') {
                fprintf(stderr, "%c", card);
                card = p->cardsPlayedC[++j];
            }
            break;
        case 3:
            fprintf(stderr, "D%c:", p->statusD);
            card = p->cardsPlayedD[j];
            while (card != '-') {
                fprintf(stderr, "%c", card);
                card = p->cardsPlayedD[++j];
            }            
            break;
    }
    fprintf(stderr, "\n");
    fflush(stderr);
}

/* Pints the status message. First prints the status and discarded cards of 
 * each player then it prints what cards this program is holding
 * @params p The player structure
 */
void print_status(struct Player *p) {

    for (int player = 0; player < p->players; ++player) {
        print_player_status(p, player);
    }

    fprintf(stderr, "You are holding:%c%c\n", p->firstCard, p->secondCard);

}

/* Compares command given with valid commands available. If command is valid 
 * returns a different code for each command. If command is not valid, exits
 * due to invalid message.
 * @params p The player structure
 * @params command the command read from stdin
 * @return A command code. Each valid command has a unique code
 */
int process_command(struct Player *p, char *command) {
    char gameover[] = "gameover", newround[] = "newround",
            yourturn[] = "yourturn", thishappened[] = "thishappened",
            replace[] = "replace", scores[] = "scores";
     
    if (!(strcmp(command, gameover))) {
        return 1;
    }
    if (!(strcmp(command, newround))) {
        return 2;
    }
    if (!(strcmp(command, yourturn))) {
        return 3;
    }
    if (!(strcmp(command, thishappened))) {
        return 4;
    }
    if (!(strcmp(command, replace))) {
        return 5;
    }
    if (!(strcmp(command, scores))) {
        return 6;
    }

    exit_player(p, INVALID_HUB);

    return -1;
}

/* Reads params given in commands expecting a single card (newround, yourturn,
 * replace) and checks to see if it's a single valid card, then returns this 
 * card. Exits if invalid.
 * @params p The player structure
 * @params param The parameter given to check if a single card
 * @return A single valid card
 */
char read_single_card(struct Player *p, char *param) {
    int paramLength;

    paramLength = strlen(param);

    if (paramLength != 1 || param[0] < '1' || param[0] > '8') {
        exit_player(p, INVALID_HUB);
    }

    return param[0];
}

/* Process new round message. This takes the param given (a card),
 * checks it, reads it as a card, initalises round, then gives the 
 * card to player as their first card.
 * @params p The player structure
 * @params param The parameter given in the newround command
 * should be a single card
 */
void new_round(struct Player *p, char *param) {
    char card;
    
    card = read_single_card(p, param);
    init_round(p);
    p->firstCard = card;
}

/* Add card played (as reported by thishappend or discarded during turn) to
 * a list of cards played by all players.
 * @params p The player structure
 * @params label the player discarded cards list to add the discarded card to
 * @params card the card to add to the players discarded card list
 */
void add_played_card(struct Player *p, char label, char card) {
    int i;

    if (label == '-' || card == '-') {
        return;
    }

    switch (label) {
        case 'A':
            for (i = 0; i < 9; ++i) {
                if (p->cardsPlayedA[i] == '-') {
                    p->cardsPlayedA[i] = card;
                    return;
                }
            }
            break;
        case 'B':
            for (i = 0; i < 9; ++i) {
                if (p->cardsPlayedB[i] == '-') {
                    p->cardsPlayedB[i] = card;
                    return;
                }
            }
            break;
        case 'C':
            for (i = 0; i < 9; ++i) {
                if (p->cardsPlayedC[i] == '-') {
                    p->cardsPlayedC[i] = card;
                    return;
                }
            }
            break;
        case 'D':
            for (i = 0; i < 9; ++i) {
                if (p->cardsPlayedD[i] == '-') {
                    p->cardsPlayedD[i] = card;
                    return;
                }
            }
    }
}

/* Process yourturn message by performing turn start actions (remove
 * protection) and then getting the appropiate move command (played card,
 * target player, guessed card) which is sent to stdout.
 * @params p The player structure
 * @params param The param passed in from command- a card
 */
void your_turn(struct Player *p, char *param) {
    char card, label;

    card = read_single_card(p, param);
    p->secondCard = card;   
    print_status(p);

    label = p->label;
    remove_protection(p, label);

    process_move(p);    
    fprintf(stdout, "%c%c%c\n", p->playCard, p->targetPlayer, p->guessedCard);
    fflush(stdout);
    fprintf(stderr, "To hub:%c%c%c\n", p->playCard, p->targetPlayer,
            p->guessedCard);

    card = p->playCard;   
    add_played_card(p, label, card);
}

/* Check to make sure that a thishappened message is valid
 * @params p The player structure
 * @params param the details of the actions reported on by (contains cards
 * and players, with the order indicitive of what action happened.
 */
void check_this_happened_params(struct Player *p, char *param) {
    int paramLength;
    char source = param[0], discard = param[1], target = param[2], 
            guess = param[3], dropper = param[5], dropped = param[6], 
            out = param [7];

    paramLength = strlen(param);

    if (paramLength != 8 || param[4] != '/') {
        exit_player(p, INVALID_HUB);    
    }

    if (check_valid_move(source, discard, target, guess, p->players)) {
        exit_player(p, INVALID_HUB);
    }

    if (check_card(dropped) || (check_player(out, p->players) &&
            (out != '-'))) {
        exit_player(p, INVALID_HUB);
    }

    if ((dropped == '-' && dropper != '-') || (dropped != '-' && 
            dropper == '-')) {
        exit_player(p, INVALID_HUB);
    }

    if (out != '-' && out != dropper) {
        exit_player(p, INVALID_HUB);
    }

    switch (discard) {
        case '8':
            if (out != source || dropper != '-' || dropped == '-') {
                exit_player(p, INVALID_HUB);
            }
            break;
        case '7':
        case '6':
        case '4':
        case '2':
            if (dropped != '-' || dropper != '-' || out != '-')  {
                exit_player(p, INVALID_HUB);
            }
    }
}

/* Set a Player as eliminated by marking it with a '-' symbol.
 * @params p The player structure
 * @params eliminatedPlayer The player to be eliminated
 */
void eliminate_player(struct Player *p, char eliminatedPlayer) {

    switch (eliminatedPlayer) {
        case 'A':
            p->statusA = '-';
            break;
        case 'B':
            p->statusB = '-';
            break;
        case 'C':
            p->statusC = '-';
            break;
        case 'D':
            p->statusD = '-';
    }

    if (eliminatedPlayer == p->label) {
        p->firstCard = '-';
        p->secondCard = '-';
    }
}

/* Process a thishappened message from the hub and take the appropriate actions
 * @params p The player structure
 * @params param the details of the actions reported on by (contains cards 
 * and players, with the order indicitive of what action happened.
 */
void this_happened(struct Player *p, char *param) {
    char sourcePlayer, playedCard, cardDropper, droppedCard, eliminatedPlayer;
    check_this_happened_params(p, param);

    sourcePlayer = param[0];
    playedCard = param[1];
    cardDropper = param[5];
    droppedCard = param[6];
    eliminatedPlayer = param[7];
    
    remove_protection(p, sourcePlayer);

    if (sourcePlayer != p->label) {
        add_played_card(p, sourcePlayer, playedCard);
    }
    
    add_played_card(p, cardDropper, droppedCard);
    
    if (eliminatedPlayer != '-') {
        eliminate_player(p, eliminatedPlayer);
    }

    if (playedCard == '4') {
        protect_player(p, sourcePlayer);
    }
    
    if (droppedCard == '4' && eliminatedPlayer == '-' && playedCard == '5') {
        protect_player(p, cardDropper);
    }
}

/* Replaces card held with card specified by replace command
 * @params p The player structure
 * @params param the param passed in- a card.
 */
void replace(struct Player *p, char *param) {
    char card;
    
    card = read_single_card(p, param);
    p->firstCard = card;
    return;
}

/* Reads and validates the scores messaged recieved from stdin
 * Exits if invalid message
 * @param p The player structure
 * @param messageIn A scores message
 */
void scores(struct Player *p, char *messageIn) {
    int scoreA = 0, scoreB = 0, scoreC = 0, scoreD = 0, scoresCount = 0,
            messageLength; 

    messageLength = strlen(messageIn);
    if (messageLength != (11 + 2 * ((p->players) - 2))) {
        exit_player(p, INVALID_HUB);
    }

    scoresCount = sscanf(messageIn, "%*s %d %d %d %d", &scoreA, &scoreB, 
            &scoreC, &scoreD);    
    if (scoresCount != p->players) {
        exit_player(p, INVALID_HUB);
    }

    if (scoreA < 0 || scoreB < 0 || scoreC < 0 || scoreD < 0 ||
            scoreA > 4 || scoreB > 4 || scoreC > 4 || scoreD > 4) {
        exit_player(p, INVALID_HUB);
    }    
    return;
}

/* Play a game of Love Letter.
 * Gets a message from stdin and performs the appropriate actions.
 * Exits if instructed via gameover or upon hubloss or invalid input.
 * @param p The player structure
 */
void play_game(struct Player *p) {
    char messageIn[23], command[23], param[23], *input, c;
    int argCount, commandCode, length;

    input = fgets(messageIn, 23, stdin);
    if (input == NULL) {
        exit_player(p, HUB_LOSS);
    }

    length = strlen(input);
    if (input[length - 1] != '\n') {
        while ((c = fgetc(stdin)) != '\n' && c != EOF);
    }

    fprintf(stderr, "From hub:%s", messageIn);
    print_status(p);
    argCount = sscanf(messageIn, "%s %[^\n]", command, param);

    commandCode = process_command(p, command);

    if (commandCode == 1) { 
        if (argCount == 1) {
            exit_player(p, NO_ERROR);
        }
        exit_player(p, INVALID_HUB);
    }
    if (argCount != 2) {
        exit_player(p, INVALID_HUB);
    }

    switch (commandCode) {
        case 2:
            new_round(p, param);
            break;
        case 3:
            your_turn(p, param);
            break;
        case 4:
            this_happened(p, param);
            break;
        case 5:
            replace(p, param);
            break;
        case 6:
            scores(p, messageIn);
            break;
    }
    print_status(p);
}

int main(int argc, char *argv[]) {
    struct Player *p = NULL;
    
    fprintf(stdout, "-");
    fflush(stdout);

    p = malloc(sizeof(*p));

    if (argc != 3) {
        exit_player(p, USEAGE_ERROR);
    }

    parse_args(p, argv);

    while (1) {
        play_game(p);
    }
}
