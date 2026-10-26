
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "train.h"

#define TEST_FILE "data/trains.txt"
#define BACKUP_FILE "data/trains_test_backup.txt"

static void prepare_test_data(void)
{
    FILE *source = fopen(TEST_FILE, "r");
    if (source != NULL) {
        FILE *backup = fopen(BACKUP_FILE, "w");
        assert(backup != NULL);

        int ch;
        while ((ch = fgetc(source)) != EOF) {
            assert(fputc(ch, backup) != EOF);
        }

        assert(fclose(source) == 0);
        assert(fclose(backup) == 0);
    }

    FILE *test_file = fopen(TEST_FILE, "w");
    assert(test_file != NULL);
    assert(fclose(test_file) == 0);

    assert(init_trains() == 0);
}

static void restore_test_data(void)
{
    FILE *backup = fopen(BACKUP_FILE, "r");

    if (backup != NULL) {
        FILE *destination = fopen(TEST_FILE, "w");
        assert(destination != NULL);

        int ch;
        while ((ch = fgetc(backup)) != EOF) {
            assert(fputc(ch, destination) != EOF);
        }

        assert(fclose(backup) == 0);
        assert(fclose(destination) == 0);
        assert(remove(BACKUP_FILE) == 0);
    } else {
        assert(remove(TEST_FILE) == 0);
    }

    (void)init_trains();
}

int main(void)
{
    prepare_test_data();

    /* Positive: add a valid train */
    assert(add_train(101, "Chennai Express", "Chennai",
                     "Bengaluru", 100, 450.0) == 1);

    Train *train = find_train_by_id(101);
    assert(train != NULL);
    assert(train->train_id == 101);
    assert(strcmp(train->train_name, "Chennai Express") == 0);
    assert(strcmp(train->source, "Chennai") == 0);
    assert(strcmp(train->destination, "Bengaluru") == 0);
    assert(train->capacity == 100);
    assert(train->base_fare == 450.0);
    assert(train->is_active == 1);

    /* Negative: duplicate train ID must be rejected */
    assert(add_train(101, "Duplicate", "A", "B",
                     50, 100.0) == 0);

    /* Negative: an unknown ID must not be found */
    assert(find_train_by_id(9999) == NULL);

    /* Boundary: zero capacity is currently accepted by the code */
    assert(add_train(102, "Zero Capacity", "A", "B",
                     0, 100.0) == 1);
    assert(find_train_by_id(102) != NULL);
    assert(find_train_by_id(102)->capacity == 0);

    /* Boundary: maximum supported number of train records */
    for (int i = 3; i <= 100; ++i) {
        assert(add_train(i, "Test Train", "A", "B",
                         1, 1.0) == 1);
    }

    /* Negative: no more than 100 trains can be added */
    assert(add_train(1010, "Extra Train", "A", "B",
                     1, 1.0) == 0);

    /* Persistence: saved records should reload */
    assert(save_trains() == 1);
    assert(init_trains() == 100);
    assert(find_train_by_id(101) != NULL);
    assert(find_train_by_id(1010) == NULL);

    /* Exercise list output */
    list_trains();

    restore_test_data();

    puts("All train unit tests passed.");
    return 0;
}
