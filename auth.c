/*
 * ================================================================
 *  AUTHENTICATION SYSTEM IN C
 *  Student-level implementation
 *  Based on Python auth_hashed.py
 * ================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <unistd.h>
#endif

// ================================================================
//  CONSTANTS AND STRUCTURES
// ================================================================

#define MAX_USERS 100
#define USERNAME_LEN 5
#define PASSWORD_LEN 8
#define SALT_LEN 5
#define HASH_LEN 20
#define PASSWORD_FILE "password.txt"

// Structure to track failed login attempts
typedef struct {
    char username[USERNAME_LEN + 1];
    int attempts;
    time_t lock_until;
    int banned;
} UserAttempt;

// Global array to track attempts (in-memory)
UserAttempt user_attempts[MAX_USERS];
int attempt_count = 0;

// ================================================================
//  UTILITY FUNCTIONS
// ================================================================

/*
 * Clear the console screen
 */
void clear_screen() {
    system("cls");  // Windows
}

/*
 * Wait for user to press Enter
 */
void press_enter() {
    printf("\n Appuyez sur Entree pour continuer...");
    getchar();
    while(getchar() != '\n');  // Clear buffer
}

/*
 * Display countdown on ONE LINE (only number changes)*/

void display_countdown(int seconds, const char* username) {
    printf("\n Verrouillage du compte '%s'\n", username);
    for (int i = seconds; i > 0; i--) {
        printf("\rBlocage (%ds) : %d ", seconds, i);
        fflush(stdout);
        #ifdef _WIN32
            Sleep(1000);  // Windows sleep in milliseconds
        #else
            sleep(1);     // Unix/Linux sleep in seconds
        #endif
    }
    printf("\rBlocage termine !                      \n");
}

// ================================================================
//  HASH FUNCTION (Simple custom hash - not cryptographic)
// ================================================================

/*
 * Simple hash function for password + salt
 * This is NOT cryptographically secure, but sufficient for educational purposes
 * Returns a hash value as unsigned long
 */
unsigned long simple_hash(const char* str) {
    unsigned long hash = 5381;
    int c;
    
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    
    return hash;
}

/*
 * Hash password with salt and convert to string
 */
void hash_password(const char* password, const char* salt, char* output) {
    char combined[PASSWORD_LEN + SALT_LEN + 1];
    sprintf(combined, "%s%s", password, salt);
    
    unsigned long hash_value = simple_hash(combined);
    sprintf(output, "%lu", hash_value);
}

/*
 * Generate random 5-digit salt
 */
void generate_salt(char* salt) {
    for (int i = 0; i < SALT_LEN; i++) {
        salt[i] = '0' + (rand() % 10);
    }
    salt[SALT_LEN] = '\0';
}

// ================================================================
//  VALIDATION FUNCTIONS
// ================================================================

/*
 * Validate username: exactly 5 lowercase letters
 * Returns 1 if valid, 0 otherwise
 */
int validate_username(const char* username) {
    if (strlen(username) != USERNAME_LEN) {
        printf(" Le username doit avoir exactement 5 caracteres\n");
        return 0;
    }
    
    for (int i = 0; i < USERNAME_LEN; i++) {
        if (username[i] < 'a' || username[i] > 'z') {
            printf("Le username doit contenir 5 lettres minuscules\n");
            return 0;
        }
    }
    
    return 1;
}

/*
 * Validate password: exactly 8 characters
 * At least 1 lowercase, 1 uppercase, 1 digit
 * Returns 1 if valid, 0 otherwise
 */
int validate_password(const char* password) {
    if (strlen(password) != PASSWORD_LEN) {
        printf("Le password doit avoir exactement 8 caracteres\n");
        return 0;
    }
    
    int has_lower = 0, has_upper = 0, has_digit = 0;
    
    for (int i = 0; i < PASSWORD_LEN; i++) {
        if (password[i] >= 'a' && password[i] <= 'z') has_lower = 1;
        if (password[i] >= 'A' && password[i] <= 'Z') has_upper = 1;
        if (password[i] >= '0' && password[i] <= '9') has_digit = 1;
    }
    
    if (!has_lower) {
        printf(" Le password doit contenir au moins une minuscule\n");
        return 0;
    }
    if (!has_upper) {
        printf(" Le password doit contenir au moins une majuscule\n");
        return 0;
    }
    if (!has_digit) {
        printf(" Le password doit contenir au moins un chiffre\n");
        return 0;
    }
    
    return 1;
}

// ================================================================
//  FILE OPERATIONS
// ================================================================

/*
 * Initialize password file (create if doesn't exist)
 */
void initialize_password_file() {
    FILE* file = fopen(PASSWORD_FILE, "a");
    if (file != NULL) {
        fclose(file);
    }
}

/*
 * Check if user exists in password file
 * Returns 1 if exists, 0 otherwise
 */
int user_exists(const char* username) {
    FILE* file = fopen(PASSWORD_FILE, "r");
    if (file == NULL) return 0;
    
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char stored_user[USERNAME_LEN + 1];
        if (sscanf(line, "%5[^:]", stored_user) == 1) {
            if (strcmp(stored_user, username) == 0) {
                fclose(file);
                return 1;
            }
        }
    }
    
    fclose(file);
    return 0;
}

/*
 * Get user data from password file
 * Returns 1 if found, 0 otherwise
 * Fills hash and salt parameters
 */
int get_user_data(const char* username, char* hash, char* salt) {
    FILE* file = fopen(PASSWORD_FILE, "r");
    if (file == NULL) return 0;
    
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char stored_user[USERNAME_LEN + 1];
        char stored_hash[100];
        char stored_salt[SALT_LEN + 1];
        
        if (sscanf(line, "%5[^:]:%99[^:]:%5s", stored_user, stored_hash, stored_salt) == 3) {
            if (strcmp(stored_user, username) == 0) {
                strcpy(hash, stored_hash);
                strcpy(salt, stored_salt);
                fclose(file);
                return 1;
            }
        }
    }
    
    fclose(file);
    return 0;
}

// ================================================================
//  ATTEMPT TRACKING AND SECURITY
// ================================================================

/*
 * Find or create user attempt record
 * Returns pointer to UserAttempt structure
 */
UserAttempt* get_user_attempt(const char* username) {
    // Search for existing record
    for (int i = 0; i < attempt_count; i++) {
        if (strcmp(user_attempts[i].username, username) == 0) {
            return &user_attempts[i];
        }
    }
    
    // Create new record if not found
    if (attempt_count < MAX_USERS) {
        strcpy(user_attempts[attempt_count].username, username);
        user_attempts[attempt_count].attempts = 0;
        user_attempts[attempt_count].lock_until = 0;
        user_attempts[attempt_count].banned = 0;
        return &user_attempts[attempt_count++];
    }
    
    return NULL;
}

/*
 * Check if account is locked or banned
 * Returns 1 if locked/banned, 0 if available
 */
int is_account_locked(const char* username, char* message) {
    UserAttempt* ua = get_user_attempt(username);
    if (ua == NULL) return 0;
    
    // Check if permanently banned
    if (ua->banned) {
        sprintf(message, " COMPTE BANNI DEFINITIVEMENT");
        return 1;
    }
    
    // Check if still locked
    time_t now = time(NULL);
    if (ua->lock_until > now) {
        int remaining = (int)(ua->lock_until - now);
        sprintf(message, "Compte verrouille (%d secondes restantes)", remaining);
        return 1;
    }
    
    return 0;
}

/*
 * Update lock time based on failed attempts
 */
void update_lock_time(const char* username) {
    UserAttempt* ua = get_user_attempt(username);
    if (ua == NULL) return;
    
    int lock_seconds = 0;
    
    if (ua->attempts == 3) {
        lock_seconds = 5;
    } else if (ua->attempts == 6) {
        lock_seconds = 10;
    } else if (ua->attempts == 9) {
        lock_seconds = 20;
    } else if (ua->attempts >= 10) {
        printf(" Trop d'echecs. COMPTE BANNI DEFINITIVEMENT.\n");
        ua->banned = 1;
        return;
    }
    
    if (lock_seconds > 0) {
        ua->lock_until = time(NULL) + lock_seconds;
        printf("Trop d'echecs. Verrouillage %d secondes.\n", lock_seconds);
        display_countdown(lock_seconds, username);
    }
}

// ================================================================
//  SIGN UP FUNCTION
// ================================================================

void signup() {
    printf("\n");
    printf("==================================================\n");
    printf(" INSCRIPTION\n");
    printf("==================================================\n");
    
    char username[100];
    char password[100];
    
    // Get and validate username
    while (1) {
        printf("Username (5 lettres minuscules): ");
        scanf("%s", username);
        getchar(); // Clear newline
        
        if (!validate_username(username)) {
            continue;
        }
        
        if (user_exists(username)) {
            printf(" Username deja utilise\n");
            continue;
        }
        
        break;
    }
    
    // Get and validate password
    while (1) {
        printf("\nExemple valide : Abc12345\n");
        printf("Password : ");
        scanf("%s", password);
        getchar(); // Clear newline
        
        if (validate_password(password)) {
            break;
        }
    }
    
    // Generate salt and hash password
    char salt[SALT_LEN + 1];
    char hash[100];
    
    generate_salt(salt);
    hash_password(password, salt, hash);
    
    // Save to file
    FILE* file = fopen(PASSWORD_FILE, "a");
    if (file != NULL) {
        fprintf(file, "%s:%s:%s\n", username, hash, salt);
        fclose(file);
        printf(" Inscription reussie !\n");
    } else {
        printf("Erreur lors de l'enregistrement\n");
    }
}

// ================================================================
//  SIGN IN FUNCTION
// ================================================================

void signin() {
    printf("\n");
    printf("==================================================\n");
    printf(" CONNEXION\n");
    printf("==================================================\n");
    
    char username[100];
    char password[100];
    char message[200];
    
    printf("Username : ");
    scanf("%s", username);
    getchar(); // Clear newline
    
    // Check if account is locked or banned
    if (is_account_locked(username, message)) {
        printf(" %s\n", message);
        return;
    }
    
    // Get user data from file
    char stored_hash[100];
    char salt[SALT_LEN + 1];
    
    if (!get_user_data(username, stored_hash, salt)) {
        printf(" Utilisateur introuvable\n");
        return;
    }
    
    // Get or create user attempt tracker
    UserAttempt* ua = get_user_attempt(username);
    if (ua == NULL) {
        printf("Erreur: impossible de creer le tracker d'attempts\n");
        return;
    }
    
    // Boucle infinie de tentatives de mot de passe
    int login_success = 0;
    
    while (!login_success) {
        printf("Password : ");
        scanf("%s", password);
        getchar(); // Clear newline
        
        // Incrémenter le compteur de tentatives
        ua->attempts++;
        
        // Hash input password with stored salt
        char input_hash[100];
        hash_password(password, salt, input_hash);
        
        // Compare hashes
        if (strcmp(input_hash, stored_hash) == 0) {
            printf(" Connexion reussie ! Bienvenue.\n");
            login_success = 1;
            
            // Reset failed attempts on successful login
            ua->attempts = 0;
        } else {
            printf(" Mot de passe incorrect\n");
            printf("Tentative %d\n", ua->attempts);
            
            // Gestion des blocages progressifs
            int lock_duration = 0;
            char lock_reason[100];
            
            if (ua->attempts == 3) {
                lock_duration = 5;
                sprintf(lock_reason, "apres 3 tentatives");
            } 
            else if (ua->attempts == 6) {
                lock_duration = 10;
                sprintf(lock_reason, "apres 6 tentatives");
            } 
            else if (ua->attempts == 7) {
                lock_duration = 15;
                sprintf(lock_reason, "apres 7 tentatives");
            } 
            else if (ua->attempts == 8) {
                lock_duration = 20;
                sprintf(lock_reason, "apres 8 tentatives");
            } 
            else if (ua->attempts >= 9) {
                // Compte banni
                ua->banned = 1;
                printf(" COMPTE BANNI DEFINITIVEMENT apres 9 tentatives!\n");
                printf(" Acces refuse.\n");
                return;
            }
            
            // Appliquer le blocage si necessaire
            if (lock_duration > 0) {
                ua->lock_until = time(NULL) + lock_duration;
                printf(" Compte verrouille pendant %d secondes %s\n", lock_duration, lock_reason);
                display_countdown(lock_duration, username);
            }
        }
    }
}

// ================================================================
//  CLEAN PASSWORD FILE
// ================================================================

void clean_password_file() {
    FILE* file = fopen(PASSWORD_FILE, "r");
    if (file == NULL) {
        printf(" Fichier introuvable\n");
        return;
    }
    
    // Read all lines
    char lines[MAX_USERS][256];
    int line_count = 0;
    
    while (fgets(lines[line_count], 256, file) && line_count < MAX_USERS) {
        // Count colons in line
        int colon_count = 0;
        for (int i = 0; lines[line_count][i] != '\0'; i++) {
            if (lines[line_count][i] == ':') {
                colon_count++;
            }
        }
        
        // Keep only valid lines (exactly 2 colons)
        if (colon_count == 2) {
            line_count++;
        }
    }
    
    fclose(file);
    
    // Rewrite file with valid lines only
    file = fopen(PASSWORD_FILE, "w");
    if (file != NULL) {
        for (int i = 0; i < line_count; i++) {
            fputs(lines[i], file);
        }
        fclose(file);
        printf(" Fichier nettoye.\n");
    }
}

// ================================================================
//  MENU DISPLAY
// ================================================================

void display_menu() {
    printf("\n");
    printf("==================================================\n");
    printf(" SYSTEME D'AUTHENTIFICATION\n");
    printf("==================================================\n");
    printf("1.  S'inscrire\n");
    printf("2.  Se connecter\n");
    printf("3.  Nettoyer le fichier\n");
    printf("4.  Quitter\n");
    printf("==================================================\n");
}

// ================================================================
//  MAIN FUNCTION
// ================================================================

int main() {
    // Seed random number generator
    srand(time(NULL));
    
    // Initialize password file
    initialize_password_file();
    
    // Clear screen
    clear_screen();
    
    // Main menu loop
    while (1) {
        display_menu();
        
        char choice[10];
        printf(" Choisissez une option : ");
        scanf("%s", choice);
        getchar(); // Clear newline
        
        if (strcmp(choice, "1") == 0) {
            signup();
        } else if (strcmp(choice, "2") == 0) {
            signin();
        } else if (strcmp(choice, "3") == 0) {
            clean_password_file();
        } else if (strcmp(choice, "4") == 0) {
            printf("\n Merci d'avoir utilise le systeme.\n");
            printf(" Au revoir !\n");
            break;
        } else {
            printf(" Option invalide.\n");
        }
        
        press_enter();
        clear_screen();
    }
    
    return 0;
}

// ================================================================
//  END OF FILE
// ================================================================
