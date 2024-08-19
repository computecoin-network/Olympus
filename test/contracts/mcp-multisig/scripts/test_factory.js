const { ethers } = require('hardhat');
const { expect } = require('chai');
const ABI = require('../artifacts/contracts/Factory.sol/Factory.json')

describe("Test MultiSig Factory & Wallet Creation", async() => {
    const [owner, outsideUser] = await ethers.getSigners();
    let multiSigFactory;
    let deployedWalletSig;
    let multiSig;

    before(async () => {
        multiSig = await ethers.getContractFactory("MultiSigWalletWithDailyLimitFactory");
        multiSigFactory = await multiSig.deploy();
        deployedWalletSig = await multiSigFactory.deployed();
        console.log(deployedWalletSig.address, "Factory Address");
        console.log("https://dev-scan.oortech.com/tx/"+deployedWalletSig.deployTransaction.hash, "Factory Creation Hash");
    });
    
    it("Should just deploy MultiSig Factory Contract and Match Bytecode", async () => {
        const data = JSON.stringify(deployedWalletSig.deployTransaction.data);
        expect(data).to.equal(data, "matched bytecode");
    });

    it("Should return correct Factory Address before moving on", async() => {
        const data = deployedWalletSig.address;
        expect(data).to.be.equal(deployedWalletSig.address);
    });

    it("should just create new wallet from factory multi sig using CREATE", async() => {
        const result = await deployedWalletSig.create([owner.address, outsideUser.address], 2, 3000)
        const data = result.data;
        expect(data).to.be.equal(result.data);
    })
    it("Should return correct number of contracts instantiations", async() => {
        const result = await deployedWalletSig.connect(owner).create([owner.address, outsideUser.address], 2, 3000)
        const contract = new ethers.Contract(result.to, ABI.abi, owner);
        const count = (await contract.getInstantiationCount(owner.address));
        expect(count).to.be.equal(2);
    });
    it("Should create wallets MultiSig from factory and return correct MultiSig Address", async() => {
        const result = await deployedWalletSig.connect(owner).create([owner.address, outsideUser.address], 2, 3000);
        const contract = new ethers.Contract(result.to, ABI.abi, owner);
        console.log(await contract.instantiations(owner.address,0), "MultiSig Address");
        })
});