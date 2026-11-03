#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <cmath>

// Function to check if a number is prime
bool isPrime(int number) {
    if (number < 2) return false;
    for (int i = 2; i <= std::sqrt(number); ++i) {
        if (number % i == 0) return false;
    }
    return true;
}

int main() {
    const int numProcesses = 10;
    const int rangeSize = 1000;

    int parentToChildPipes[numProcesses][2];
    int childToParentPipes[numProcesses][2];
    pid_t processIds[numProcesses];

    // Create bidirectional pipes for each process
    for (int i = 0; i < numProcesses; ++i) {
        if (pipe(parentToChildPipes[i]) == -1 || pipe(childToParentPipes[i]) == -1) {
            std::cerr << "Pipe creation failed.\n";
            return 1;
        }
    }

    // Fork the processes
    for (int i = 0; i < numProcesses; ++i) {
        processIds[i] = fork();

        if (processIds[i] < 0) {
            std::cerr << "Fork failed.\n";
            return 1;
        }

        if (processIds[i] == 0) { // Child process
            close(parentToChildPipes[i][1]); // Close write end from parent
            close(childToParentPipes[i][0]); // Close read end to parent

            int bounds[2];
            // Receive range from parent
            read(parentToChildPipes[i][0], bounds, sizeof(bounds));
            close(parentToChildPipes[i][0]);

            int startRange = bounds[0];
            int endRange = bounds[1];
            std::vector<int> primeNumbers;

            // Find primes in the assigned range
            for (int num = startRange; num <= endRange; ++num) {
                if (isPrime(num)) {
                    primeNumbers.push_back(num);
                }
            }

            // Send primes back to parent
            int primeCount = primeNumbers.size();
            write(childToParentPipes[i][1], &primeCount, sizeof(primeCount));
            if (primeCount > 0) {
                write(childToParentPipes[i][1], primeNumbers.data(), primeCount * sizeof(int));
            }
            
            close(childToParentPipes[i][1]);
            exit(0);
        }
    }

    // Parent process
    for (int i = 0; i < numProcesses; ++i) {
        close(parentToChildPipes[i][0]); 
        close(childToParentPipes[i][1]); 

        int startRange = i * rangeSize + 1;
        int endRange = (i + 1) * rangeSize;
        int bounds[2] = {startRange, endRange};

        // Send interval bounds to each child
        write(parentToChildPipes[i][1], bounds, sizeof(bounds));
        close(parentToChildPipes[i][1]);
    }

    // Read and print primes from each child
    for (int i = 0; i < numProcesses; ++i) {
        int primeCount;
        read(childToParentPipes[i][0], &primeCount, sizeof(primeCount));

        if (primeCount > 0) {
            std::vector<int> primeNumbers(primeCount);
            read(childToParentPipes[i][0], primeNumbers.data(), primeCount * sizeof(int));

            std::cout << "Primes from child " << i + 1 
                      << " (Range " << (i * rangeSize + 1) << "-" << ((i + 1) * rangeSize) << "): ";
            for (int prime : primeNumbers) {
                std::cout << prime << " ";
            }
            std::cout << "\n\n";
        }
        close(childToParentPipes[i][0]);
        waitpid(processIds[i], nullptr, 0); // Wait for child execution to complete
    }

    return 0;
}
