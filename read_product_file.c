#include "read_product_file.h"

int read_product_file(char *filename, int *num_products_ptr, Product** products_ptr) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("Error opening file");
        return 1;
    }

    // Read first line
    char *line = NULL;
    size_t len = 0;
    ssize_t read = getline(&line, &len, fp);
    if (read < 0) {
        perror("Error reading file");
        return 1;
    }

    int num_products = atoi(line);
    *num_products_ptr = num_products;
    printf("%s Trading %d products:", LOG_PREFIX, num_products);

    // Allocate memory for products array
    *products_ptr = calloc(num_products, sizeof(Product));
    if (*products_ptr == NULL) {
        perror("Error allocating memory");
        return 1;
    }
    // Read products into array
    for (int i = 0; i < num_products; i++) {
        read = getline(&line, &len, fp);
        if (read < 0) {
            perror("Error reading file");
            return 1;
        }
        int line_len = strlen(line);
        if (line_len > 0 && line[line_len - 1] == '\n') {
            line[line_len - 1] = '\0'; // Remove newline
            line_len--;
        }
        if (line_len > 16) {
            perror("Error: Product name too long\n");
            return 1;
        }
        // char *product = malloc((line_len + 1) * sizeof(char));
        // if (product == NULL) {
        //     perror("Error allocating memory");
        //     return 1;
        // }
        // strcpy(product, line); // Copy line into product
        // Set product values
        //(*products_ptr)[i].name = product;
        (*products_ptr)[i].name = line;
        (*products_ptr)[i].buy_levels = 0;
        (*products_ptr)[i].sell_levels = 0;
        (*products_ptr)[i].product_index = i;
        line = NULL;
        len = 0;
    }

    // Printing product names
    for (int i = 0; i < num_products; i++) {
        printf(" %s", (*products_ptr)[i].name);
    }
    printf("\n");
    // free(*products_ptr);

    fclose(fp);
    return 0;
}
