/*
 * ================================================================
 *  BRUTE FORCE ATTACK SYSTEM
 *  Attaque par force brute contre le systeme d'authentification
 * ================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ================================================================
//  CONSTANTS
// ================================================================

#define USERNAME_LEN 5
#define PASSWORD_LEN 8
#define SALT_LEN 5
#define PASSWORD_FILE "password.txt"

// Caracteristiques du brute force
const char USERNAME_CHARSET[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
const char PASSWORD_CHARSET[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789@#$&";

// ================================================================
//  HASH FUNCTION (Same as auth.c)
// ================================================================

/*
 * Simple hash function - must match auth.c implementation
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
 * Hash password with salt
 */
void hash_password(const char* password, const char* salt, char* output) {
    char combined[PASSWORD_LEN + SALT_LEN + 1];
    sprintf(combined, "%s%s", password, salt);
    
    unsigned long hash_value = simple_hash(combined);
    sprintf(output, "%lu", hash_value);
}

// ================================================================
//  FILE OPERATIONS
// ================================================================

/*
 * Get user data from password file
 * Returns 1 if found, 0 otherwise
 */
int get_user_data(const char* username, char* hash, char* salt) {
    FILE* file = fopen(PASSWORD_FILE, "r");
    if (file == NULL) {
        printf("Erreur: impossible d'ouvrir %s\n", PASSWORD_FILE);
        return 0;
    }
    
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

/*
 * Get all usernames from password file
 */
int get_all_usernames(char usernames[][USERNAME_LEN + 1], int max_users) {
    FILE* file = fopen(PASSWORD_FILE, "r");
    if (file == NULL) {
        return 0;
    }
    
    char line[256];
    int count = 0;
    
    while (fgets(line, sizeof(line), file) && count < max_users) {
        char stored_user[USERNAME_LEN + 1];
        if (sscanf(line, "%5[^:]", stored_user) == 1) {
            strcpy(usernames[count], stored_user);
            count++;
        }
    }
    
    fclose(file);
    return count;
}

// ================================================================
//  BRUTE FORCE ENGINE
// ================================================================

/*
 * Generate next combination for brute force
 * Returns 1 if successful, 0 if all combinations tried
 */
int next_combination(char* str, int length, const char* charset) {
    int charset_len = strlen(charset);
    int pos = length - 1;
    
    while (pos >= 0) {
        // Find current character index
        int idx = 0;
        while (idx < charset_len && charset[idx] != str[pos]) {
            idx++;
        }
        
        // Try next character
        if (idx < charset_len - 1) {
            str[pos] = charset[idx + 1];
            return 1;
        } else {
            // Reset this position and carry to next
            str[pos] = charset[0];
            pos--;
        }
    }
    
    return 0; // All combinations exhausted
}

/*
 * Initialize combination string with first characters
 */
void init_combination(char* str, int length, const char* charset) {
    for (int i = 0; i < length; i++) {
        str[i] = charset[0];
    }
    str[length] = '\0';
}

/*
 * Try to crack password knowing the username
 */
int crack_password_with_username(const char* username, char* cracked_password) {
    char stored_hash[100];
    char salt[SALT_LEN + 1];
    
    // Get user data
    if (!get_user_data(username, stored_hash, salt)) {
        printf("Utilisateur '%s' introuvable\n", username);
        return 0;
    }
    
    printf("\n=== ATTAQUE MODE 2: Username connu ===\n");
    printf("Cible: %s\n", username);
    printf("Hash cible: %s\n", stored_hash);
    printf("Salt: %s\n", salt);
    printf("Demarrage de l'attaque par force brute...\n\n");
    
    char password[PASSWORD_LEN + 1];
    char computed_hash[100];
    unsigned long long attempts = 0;
    time_t start_time = time(NULL);
    
    // Initialize first combination
    init_combination(password, PASSWORD_LEN, PASSWORD_CHARSET);
    
    do {
        attempts++;
        
        // Compute hash for current password attempt
        hash_password(password, salt, computed_hash);
        
        // Check if match
        if (strcmp(computed_hash, stored_hash) == 0) {
            time_t end_time = time(NULL);
            double elapsed = difftime(end_time, start_time);
            
            printf("\n");
            printf("=================================================\n");
            printf("*** MOT DE PASSE TROUVE ! ***\n");
            printf("=================================================\n");
            printf("Username: %s\n", username);
            printf("Password: %s\n", password);
            printf("Tentatives: %llu\n", attempts);
            printf("Temps ecoule: %.0f secondes\n", elapsed);
            printf("=================================================\n");
            
            strcpy(cracked_password, password);
            return 1;
        }
        
        // Display progress every 10000 attempts
        if (attempts % 10000 == 0) {
            printf("Tentatives: %llu - Test: %s\r", attempts, password);
            fflush(stdout);
        }
        
    } while (next_combination(password, PASSWORD_LEN, PASSWORD_CHARSET));
    
    printf("\nEchec: toutes les combinaisons testees sans succes.\n");
    return 0;
}

/*
 * Try to crack both username and password
 */
int crack_full_credentials(char* cracked_username, char* cracked_password) {
    printf("\n=== ATTAQUE MODE 1: Username et Password inconnus ===\n");
    printf("Recherche de credentials valides...\n");
    printf("Cette attaque peut prendre beaucoup de temps!\n\n");
    
    char username[USERNAME_LEN + 1];
    char password[PASSWORD_LEN + 1];
    unsigned long long username_attempts = 0;
    unsigned long long password_attempts = 0;
    time_t start_time = time(NULL);
    
    // Initialize first username combination
    init_combination(username, USERNAME_LEN, USERNAME_CHARSET);
    
    do {
        username_attempts++;
        
        // Check if username exists
        char stored_hash[100];
        char salt[SALT_LEN + 1];
        
        if (get_user_data(username, stored_hash, salt)) {
            printf("\nUsername trouve: %s\n", username);
            printf("Tentative de crack du password...\n");
            
            // Initialize first password combination
            init_combination(password, PASSWORD_LEN, PASSWORD_CHARSET);
            password_attempts = 0;
            
            do {
                password_attempts++;
                
                // Compute hash for current password attempt
                char computed_hash[100];
                hash_password(password, salt, computed_hash);
                
                // Check if match
                if (strcmp(computed_hash, stored_hash) == 0) {
                    time_t end_time = time(NULL);
                    double elapsed = difftime(end_time, start_time);
                    
                    printf("\n");
                    printf("=================================================\n");
                    printf("*** CREDENTIALS COMPLETES TROUVEES ! ***\n");
                    printf("=================================================\n");
                    printf("Username: %s\n", username);
                    printf("Password: %s\n", password);
                    printf("Tentatives username: %llu\n", username_attempts);
                    printf("Tentatives password: %llu\n", password_attempts);
                    printf("Temps total: %.0f secondes\n", elapsed);
                    printf("=================================================\n");
                    
                    strcpy(cracked_username, username);
                    strcpy(cracked_password, password);
                    return 1;
                }
                
                // Display progress
                if (password_attempts % 10000 == 0) {
                    printf("  Password: %llu - Test: %s\r", password_attempts, password);
                    fflush(stdout);
                }
                
            } while (next_combination(password, PASSWORD_LEN, PASSWORD_CHARSET));
            
            printf("\nPassword non trouve pour %s, passage au username suivant...\n", username);
        }
        
        // Display progress for username attempts
        if (username_attempts % 1000 == 0) {
            printf("Username: %llu - Test: %s\r", username_attempts, username);
            fflush(stdout);
        }
        
    } while (next_combination(username, USERNAME_LEN, USERNAME_CHARSET));
    
    printf("\nEchec: aucun credential valide trouve.\n");
    return 0;
}

// ================================================================
//  OPTIMIZED ATTACK WITH DICTIONARY
// ================================================================

/*
 * Try common passwords first (optimization)
 * This is a simple dictionary attack before full brute force
 */
int dictionary_attack(const char* username, char* cracked_password) {
    char stored_hash[100];
    char salt[SALT_LEN + 1];
    
    if (!get_user_data(username, stored_hash, salt)) {
        return 0;
    }
    
    // Common password patterns (8 characters)
    const char* common_passwords[] = {
        "Password", "12345678", "Abcd1234", "Admin123",
        "Test1234", "User1234", "Pass1234", "Qwerty12",
        "Azerty12", "Welcome1", "Login123", "Secret12",
        NULL
    };
    
    printf("Tentative d'attaque par dictionnaire...\n");
    
    char computed_hash[100];
    for (int i = 0; common_passwords[i] != NULL; i++) {
        hash_password(common_passwords[i], salt, computed_hash);
        
        if (strcmp(computed_hash, stored_hash) == 0) {
            printf("*** MOT DE PASSE TROUVE (dictionnaire) ! ***\n");
            printf("Password: %s\n", common_passwords[i]);
            strcpy(cracked_password, common_passwords[i]);
            return 1;
        }
    }
    
    printf("Dictionnaire echoue, passage au brute force...\n");
    return 0;
}

// ================================================================
//  DISPLAY AND MENU FUNCTIONS
// ================================================================

void display_menu() {
    printf("\n");
    printf("==================================================\n");
    printf(" SYSTEME D'ATTAQUE PAR BRUTE FORCE\n");
    printf("==================================================\n");
    printf("1. Attaque MODE 1: Username + Password inconnus\n");
    printf("2. Attaque MODE 2: Username connu (cracker password)\n");
    printf("3. Test dictionnaire + brute force\n");
    printf("4. Afficher les usernames dans le fichier\n");
    printf("5. Quitter\n");
    printf("==================================================\n");
}

void display_usernames() {
    char usernames[100][USERNAME_LEN + 1];
    int count = get_all_usernames(usernames, 100);
    
    printf("\n=== Usernames dans %s ===\n", PASSWORD_FILE);
    if (count == 0) {
        printf("Aucun utilisateur trouve.\n");
    } else {
        for (int i = 0; i < count; i++) {
            printf("%d. %s\n", i + 1, usernames[i]);
        }
    }
}

// ================================================================
//  MAIN FUNCTION
// ================================================================

int main() {
    printf("\n");
    printf("==================================================\n");
    printf(" BRUTE FORCE ATTACK TOOL\n");
    printf("==================================================\n");
    
    while (1) {
        display_menu();
        
        int choice;
        printf("Choisissez une option: ");
        scanf("%d", &choice);
        getchar(); // Clear newline
        
        if (choice == 1) {
            // Full brute force attack
            char username[USERNAME_LEN + 1];
            char password[PASSWORD_LEN + 1];
            
            printf("\n*** ATTENTION ***\n");
            printf("Cette attaque peut prendre TRES longtemps!\n");
            printf("Nombre de combinaisons possibles:\n");
            printf("- Usernames: 62^5 = 916,132,832\n");
            printf("- Passwords: 66^8 = 360,040,606,269,696\n");
            printf("\nVoulez-vous vraiment continuer? (o/n): ");
            
            char confirm;
            scanf("%c", &confirm);
            getchar();
            
            if (confirm == 'o' || confirm == 'O') {
                crack_full_credentials(username, password);
            } else {
                printf("Attaque annulee.\n");
            }
            
        } else if (choice == 2) {
            // Attack with known username
            char username[USERNAME_LEN + 1];
            char password[PASSWORD_LEN + 1];
            
            printf("\nEntrez le username connu (5 caracteres): ");
            scanf("%s", username);
            getchar();
            
            if (strlen(username) != USERNAME_LEN) {
                printf("Erreur: le username doit avoir exactement 5 caracteres!\n");
                continue;
            }
            
            printf("\nNombre de combinaisons a tester: 66^8 = 360,040,606,269,696\n");
            printf("Cela peut prendre du temps. Continuer? (o/n): ");
            
            char confirm;
            scanf("%c", &confirm);
            getchar();
            
            if (confirm == 'o' || confirm == 'O') {
                crack_password_with_username(username, password);
            } else {
                printf("Attaque annulee.\n");
            }
            
        } else if (choice == 3) {
            // Dictionary + brute force attack
            char username[USERNAME_LEN + 1];
            char password[PASSWORD_LEN + 1];
            
            printf("\nEntrez le username: ");
            scanf("%s", username);
            getchar();
            
            if (!dictionary_attack(username, password)) {
                crack_password_with_username(username, password);
            }
            
        } else if (choice == 4) {
            // Display usernames
            display_usernames();
            
        } else if (choice == 5) {
            printf("\nFermeture du programme.\n");
            break;
            
        } else {
            printf("Option invalide.\n");
        }
        
        printf("\nAppuyez sur Entree pour continuer...");
        getchar();
    }
    
    return 0;
}

// ================================================================
//  END OF FILE
// ================================================================
