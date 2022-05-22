cleos -u http://wax.cryptolions.io set contract endlessnftft /root/eos /root/eos/build/artifacts/endlessnftft.wasm /root/eos/build/artifacts/endlessnftft.abi -p endlessnftft@active

cleos -u https://wax.greymass.com push transaction burnnft.json

cleos -u https://wax.greymass.com push transaction settemplate.json

cleos -u https://wax.greymass.com push transaction askburn.json

cleos -u https://wax.greymass.com push transaction delegatebw.json