#include <iostream>
#include <vector>
#include <algorithm>
#include <omp.h>
#include <fstream>

#define MAX_FOR_THREADS 4

void display_matrix(std::vector<std::vector<char>> matrix)
{
    for (int x = 0; x < matrix.size(); x++)
    {
        for (int y = 0; y < matrix[0].size(); y++)
        {
            std::cout << matrix[x][y] << " ";
        }
        std::cout << std::endl;
    }
}

std::vector<std::vector<char>> create_matrix(const std::string &secret_key)
{
    std::string key = secret_key;
    std::transform(key.begin(), key.end(), key.begin(), ::toupper);
    std::string result;

    for (auto c : key)
    {
        if (result.find(c) == std::string::npos)
        {
            result += c;
        }
    }

    key = result;
    std::string alphabet = "ABCDEFGHIKLMNOPQRSTUVWXYZ";

    for (char c : key)
    {
        alphabet.erase(std::remove(alphabet.begin(), alphabet.end(), c), alphabet.end());
    }
    std::vector<std::vector<char>> matrix(5, std::vector<char>(5));
    int index = 0;
    int key_size = key.size();

    for (int i = 0; i < 5; ++i)
    {
        for (int j = 0; j < 5 && index < key_size; ++j)
        {
            matrix[i][j] = key[index++];
        }
    }

    for (int i = 0; i < 5; ++i)
    {
        for (int j = 0; j < 5 && !alphabet.empty(); ++j)
        {
            if (matrix[i][j] == 0)
            {
                {
                    matrix[i][j] = alphabet[0];
                    alphabet.erase(alphabet.begin());
                }
            }
        }
    }
    return matrix;
}

std::pair<int, int> find_position(const std::vector<std::vector<char>> &matrix, char letter)
{
    for (int i = 0; i < 5; ++i)
    {
        for (int j = 0; j < 5; ++j)
        {
            if (matrix[i][j] == letter)
            {
                return std::make_pair(i, j);
            }
        }
    }
    return std::make_pair(-1, -1);
}

std::string encrypt(const std::vector<std::vector<char>> &matrix, const std::string &plaintext)
{
    std::string text = plaintext;
    std::transform(text.begin(), text.end(), text.begin(), ::toupper);
    std::replace(text.begin(), text.end(), 'J', 'I');
    std::string ciphertext;

#pragma omp parallel for num_threads(MAX_FOR_THREADS)
    for (size_t i = 0; i < text.length(); i += 2)
    {
        char a = text[i];
        char b = i + 1 < text.length() ? text[i + 1] : 'X';
        if (a == b)
        {
            b = 'X';
        }
        auto pos_a = find_position(matrix, a);
        auto pos_b = find_position(matrix, b);
        int row1 = pos_a.first, col1 = pos_a.second;
        int row2 = pos_b.first, col2 = pos_b.second;
        if (row1 == row2)
        {
            {
                ciphertext += matrix[row1][(col1 + 1) % 5];
                ciphertext += matrix[row2][(col2 + 1) % 5];
            }
        }
        else if (col1 == col2)
        {
            {
                ciphertext += matrix[(row1 + 1) % 5][col1];
                ciphertext += matrix[(row2 + 1) % 5][col2];
            }
        }
        else
        {
            {
                ciphertext += matrix[row1][col2];
                ciphertext += matrix[row2][col1];
            }
        }
    }
    return ciphertext;
}

std::string decrypt(const std::vector<std::vector<char>> &matrix, const std::string &ciphertext)
{
    double start_time = omp_get_wtime();
    std::string text = ciphertext;
    std::transform(text.begin(), text.end(), text.begin(), ::toupper);
    std::replace(text.begin(), text.end(), 'J', 'I');
    std::string decrypted;

#pragma omp parallel for num_threads(MAX_FOR_THREADS)
    for (size_t i = 0; i < text.length(); i += 2)
    {
        char a = text[i];
        char b = i + 1 < text.length() ? text[i + 1] : 'X';
        if (a == b)
        {
            b = 'X';
        }
        auto pos_a = find_position(matrix, a);
        auto pos_b = find_position(matrix, b);
        int row1 = pos_a.first, col1 = pos_a.second;
        int row2 = pos_b.first, col2 = pos_b.second;
        if (row1 == row2)
        {
            {
                decrypted += matrix[row1][(col1 - 1 + 5) % 5];
                decrypted += matrix[row2][(col2 - 1 + 5) % 5];
            }
        }
        else if (col1 == col2)
        {
            {
                decrypted += matrix[(row1 - 1 + 5) % 5][col1];
                decrypted += matrix[(row2 - 1 + 5) % 5][col2];
            }
        }
        else
        {
            {
                decrypted += matrix[row1][col2];
                decrypted += matrix[row2][col1];
            }
        }
    }
    double end_time = omp_get_wtime();
    std::cout << "Time taken for decrypt: " << end_time - start_time << " seconds" << std::endl;
    return decrypted;
}

int main()
{
    std::string secret_key = "monarchy";
    auto matrix = create_matrix(secret_key);

    std::ifstream input_file("words_alpha.txt");
    if (!input_file.is_open())
    {
        std::cerr << "Failed to open input file." << std::endl;
        return 1;
    }

    std::string word;
    int word_count = 0;
    double total_encryption_time = 0.0;

    double start_time_encryption = omp_get_wtime();

#pragma omp parallel num_threads(4) default(none) shared(input_file, matrix) private(word)
    {
#pragma omp single
        {
            while (input_file >> word)
            {
#pragma omp task
                {
                    std::string encrypted = encrypt(matrix, word);
                }
            }
        }
    }

    double end_time_encryption = omp_get_wtime();
    total_encryption_time += end_time_encryption - start_time_encryption;

    std::cout << "Time taken for encryption: " << total_encryption_time << " seconds" << std::endl;

    return 0;
}