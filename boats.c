//----Boat Management
//-----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_BOATS 120
#define NAME_LEN 128
#define LICENSE_TAG_LEN 32
#define LINE_LEN 256
//-----------------------------------------------------------------------------
// Definitions
typedef enum { SLIP, LAND, TRAILOR, STORAGE } Location; // Boat location types
// Location-specific information stored as a union
typedef union {
    int slip_num;
    char bay_letter;
    char license_tag[LICENSE_TAG_LEN];
    int storage_num;
} LocationInfo;
// Boat structure containing general and location-specific data
typedef struct {
    char name[NAME_LEN];
    float length;
    Location location_type;
    LocationInfo location_info;
    float amount_owed;
} Boat;
Boat* boats[MAX_BOATS];
int boat_count = 0;
//-----------------------------------------------------------------------------
// Functions
// Convert a string to lowercase
void toLowercase(char* str) {
    while (*str) {
        *str = tolower((unsigned char)*str);
        str++;
    }
}
// Compare boats by name
int compareBoats(const void* a, const void* b) {
    Boat* ba = *(Boat**)a;
    Boat* bb = *(Boat**)b;
    return strcasecmp(ba->name, bb->name);
}
// Sort boats alphabetically
void sortBoats() {
    qsort(boats, boat_count, sizeof(Boat*), compareBoats);
}
// Find index of a boat
int find_boat_index(const char* name) {
    for (int i = 0; i < boat_count; i++) {
        if (strcasecmp(boats[i]->name, name) == 0)
            return i;
    }
    return -1;
}
// Print details of a boat
void printBoats(Boat* b) {
    printf("%-20s %3.0f'  ", b->name, b->length);
    switch (b->location_type) {
        case SLIP:
            printf("slip   # %-3d", b->location_info.slip_num);
            break;
        case LAND:
            printf("land     %c  ", b->location_info.bay_letter);
            break;
        case TRAILOR:
            printf("trailor %-6s", b->location_info.license_tag);
            break;
        case STORAGE:
            printf("storage # %-3d", b->location_info.storage_num);
            break;
    }
    printf("   Owes $%7.2f\n", b->amount_owed);
}
// Print all boats 
void printInventory() {
    for (int i = 0; i < boat_count; i++)
        printBoats(boats[i]);
}
// Remove a boat
void removeBoat(const char* name) {
    int index = find_boat_index(name);
    if (index == -1) {
        printf("No boat with that name\n");
        return;
    }

    free(boats[index]);
    for (int i = index; i < boat_count - 1; i++)
        boats[i] = boats[i + 1];
    boat_count--;
}
// Accept payment from an owner
void acceptPayment(const char* name, float amount) {
    int index = find_boat_index(name);
    if (index == -1) {
        printf("No boat with that name\n");
        return;
    }

    Boat* b = boats[index];
    if (amount > b->amount_owed) {
        printf("That is more than the amount owed, $%.2f\n", b->amount_owed);
    } else {
        b->amount_owed -= amount;
    }
}
// Apply monthly charges
void monthlyUpdate() {
    for (int i = 0; i < boat_count; i++) {
        Boat* b = boats[i];
        float rate = 0;
        switch (b->location_type) {
            case SLIP: rate = 12.5f; break;
            case LAND: rate = 14.0f; break;
            case TRAILOR: rate = 25.0f; break;
            case STORAGE: rate = 11.2f; break;
        }
        b->amount_owed += b->length * rate;
    }
}
// Load file in csv format
void load_csv(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open file");
        return;
    }

    char line[LINE_LEN];
    while (fgets(line, sizeof(line), fp)) {
        Boat* b = malloc(sizeof(Boat));
        if (!b) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }

        char type[16], extra[LICENSE_TAG_LEN];
        if (sscanf(line, "%127[^,],%f,%15[^,],%31[^,],%f", b->name, &b->length, type, extra, &b->amount_owed) == 5) {
            if (strcmp(type, "slip") == 0) {
                b->location_type = SLIP;
                b->location_info.slip_num = atoi(extra);
            } else if (strcmp(type, "land") == 0) {
                b->location_type = LAND;
                b->location_info.bay_letter = extra[0];
            } else if (strcmp(type, "trailor") == 0) {
                b->location_type = TRAILOR;
                strncpy(b->location_info.license_tag, extra, LICENSE_TAG_LEN);
            } else if (strcmp(type, "storage") == 0) {
                b->location_type = STORAGE;
                b->location_info.storage_num = atoi(extra);
            }
            boats[boat_count++] = b;
        } else {
            free(b);
        }
    }

    fclose(fp);
    sortBoats();
}
// Save file
void save_csv(const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to save file");
        return;
    }

    for (int i = 0; i < boat_count; i++) {
        Boat* b = boats[i];
        switch (b->location_type) {
            case SLIP:
                fprintf(fp, "%s,%.0f,slip,%d,%.2f\n", b->name, b->length, b->location_info.slip_num, b->amount_owed);
                break;
            case LAND:
                fprintf(fp, "%s,%.0f,land,%c,%.2f\n", b->name, b->length, b->location_info.bay_letter, b->amount_owed);
                break;
            case TRAILOR:
                fprintf(fp, "%s,%.0f,trailor,%s,%.2f\n", b->name, b->length, b->location_info.license_tag, b->amount_owed);
                break;
            case STORAGE:
                fprintf(fp, "%s,%.0f,storage,%d,%.2f\n", b->name, b->length, b->location_info.storage_num, b->amount_owed);
                break;
        }
    }

    fclose(fp);
}
// Add a boat to the system from a string
void add_boat_from_string(const char* csv) {
    if (boat_count >= MAX_BOATS) {
        printf("Marina is full.\n");
        return;
    }

    Boat* b = malloc(sizeof(Boat));
    if (!b) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    char type[16], extra[LICENSE_TAG_LEN];
    if (sscanf(csv, "%127[^,],%f,%15[^,],%31[^,],%f", b->name, &b->length, type, extra, &b->amount_owed) == 5) {
        if (strcmp(type, "slip") == 0) {
            b->location_type = SLIP;
            b->location_info.slip_num = atoi(extra);
        } else if (strcmp(type, "land") == 0) {
            b->location_type = LAND;
            b->location_info.bay_letter = extra[0];
        } else if (strcmp(type, "trailor") == 0) {
            b->location_type = TRAILOR;
            strncpy(b->location_info.license_tag, extra, LICENSE_TAG_LEN);
        } else if (strcmp(type, "storage") == 0) {
            b->location_type = STORAGE;
            b->location_info.storage_num = atoi(extra);
        }
        boats[boat_count++] = b;
        sortBoats();
    } else {
        free(b);
        printf("Invalid boat data format.\n");
    }
}
//-----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s BoatData.csv\n", argv[0]);
        return 1;
    }

    load_csv(argv[1]);

    printf("Welcome to the Boat Management System\n-------------------------------------\n");

    char input[LINE_LEN];
    while (1) {
        printf("\n(I)nventory, (A)dd, (R)emove, (P)ayment, (M)onth, e(X)it : ");
        fgets(input, sizeof(input), stdin);
        char option = tolower(input[0]);

        switch (option) {
            case 'i':
                printInventory();
                break;
            case 'a':
                printf("Please enter the boat data in CSV format                 : ");
                fgets(input, sizeof(input), stdin);
                input[strcspn(input, "\n")] = '\0';
                add_boat_from_string(input);
                break;
            case 'r':
                printf("Please enter the boat name                               : ");
                fgets(input, sizeof(input), stdin);
                input[strcspn(input, "\n")] = '\0';
                removeBoat(input);
                break;
            case 'p':
                printf("Please enter the boat name                               : ");
                fgets(input, sizeof(input), stdin);
                input[strcspn(input, "\n")] = '\0';
                char name[NAME_LEN];
                strcpy(name, input);
                int index = find_boat_index(name);
                if (index == -1) {
                    printf("No boat with that name\n");
                    break;
                }
                printf("Please enter the amount to be paid                       : ");
                fgets(input, sizeof(input), stdin);
                float amount = atof(input);
                acceptPayment(name, amount);
                break;
            case 'm':
                monthlyUpdate();
                break;
            case 'x':
                printf("\nExiting the Boat Management System\n");
                save_csv(argv[1]);
                for (int i = 0; i < boat_count; i++)
                    free(boats[i]);
                return 0;
            default:
                printf("Invalid option %c\n", input[0]);
        }
    }
}
//-----------------------------------------------------------------------------
