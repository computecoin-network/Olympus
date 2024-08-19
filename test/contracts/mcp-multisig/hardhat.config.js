require("@nomiclabs/hardhat-waffle")
//require("@nomiclabs/hardhat-etherscan")
//require('@openzeppelin/hardhat-upgrades')
require("dotenv").config()
/**
 * @type import('hardhat/config').HardhatUserConfig
 */
module.exports = {
  solidity: {
    compilers: [
      {
        version: '0.5.7',
        settings: {
          optimizer: {
            enabled: true,
            runs: 200,
          },
        },
      },
      {
        version: '0.6.0',
        settings: {
          optimizer: {
            enabled: true,
            runs: 200,
          },
        },
      },
      {
        version: '0.4.15',
        settings: {
          optimizer: {
            enabled: true,
            runs: 200,
          },
        },
      },
      {
        version: '0.4.18',
        settings: {
          optimizer: {
            enabled: true,
            runs: 200,
          },
        },
      },
      {
        version: '0.5.16',
        settings: {
          optimizer: {
            enabled: true,
            runs: 200,
          },
        },
      },
      {
        version: '0.8.10',
        settings: {
          optimizer: {
            enabled: true,
            runs: 200,
          },
        },
      },
      {
        version: '0.7.0',
        settings: {
          optimizer: {
            enabled: true,
            runs: 200,
          },
        },
      },
    ],
  },
  networks: {
    ccnbeta: {
      url: process.env.CCNBETA_URL,
      allowUnlimitedContractSize: true,
      accounts: [
        process.env.CCNBETA_PRIVATE_KEY,
        process.env.OUTSIDE_USER,
      ]
    },
    local: {
      url: "http://localhost:8545",
      allowUnlimitedContractSize: true,
      accounts: [
        process.env.LOCAL_PRIVATE_KEY,
        process.env.OUTSIDE_USER,
      ]
    }
  },
};
