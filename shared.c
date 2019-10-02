/* shared.c - Michael Scotson
 */

int check_card(char card) {
    if ((card < '1' || card > '8') && card != '-') {
        return 1;
    }
    return 0;
}

int check_player(char player, int players) {
    if ((player > (players + 64)) || player < 'A') {
        return 1;
    }
    return 0;
}


int check_valid_move(char source, char discard, char target, char guess, 
        int players) {

    if (((source == target) && discard != '5') || (discard == '5' &&
            target == '-')) {
        return 1;
    }

    if (check_player(source, players) || check_card(discard) || check_card(guess)) {
        return 1;
    }

    if (discard != '1' && guess != '-') {
        return 1;
    }

    switch (discard) {
        case '6':
        case '5':
        case '3':
            if (check_player(target, players)) {
                if (target != '-') {
                    return 1;
                }
            }
            break;
        case '1':
            if (target == '-' && guess != '-') {
                return 1;
            }
            if (guess == '-' && target != '-') {
                return 1;
            }
            break;
        default: 
            if ((guess != '-') || (target != '-')) {
                return 1;
            }
    }
    return 0;
}
