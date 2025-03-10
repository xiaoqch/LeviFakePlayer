
# for lse-quickjs
cd .\lse-api
npm i -y
npm run test
cd ..

mkdir -Force .\bin\
cp -r -Force lse-api\dist\lfp4lsejs-test\ .\bin\

cp .\lse-api\manifest.js.json .\bin\lfp4lsejs-test\manifest.json

