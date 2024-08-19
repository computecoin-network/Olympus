# MultiSig Wallet on CCNBETA

![Horizontal Logo (TM)](https://github.com/oort-tech/Olympus/assets/41552663/bc195389-b1ec-4d96-9f23-bb90415b1e36)

### Project Setup and Testing Guide

This repository contains the source code for the mcp-dydx project. Follow the steps below to set up the project, compile the code, and run tests.

### Requirements
- Node.js version 20 or later (20 is recommended)

### Installation
1. Navigate to the source directory in your terminal.
2. Run the following command to install dependencies using npm:

    ```bash
    npm i --save-dev
    ```
3. Copy .env.example to .env and fill required values

### Compilation & Deployment
Compile the project using the following command:

```bash
npm run test_ccnbeta
```

#### Alternative

You may run the provided shell script
```bash
./test_ccnbeta.sh # Deploy Factory and Test Protocol to CCNBETA