#include <iostream>
#include "Utils/DowndloadProgress/DownloadProgress.h"

int main()
{
    MetaDataFile meta("../test.gitabic");
    DownloadProgress dp(meta);
    auto v = dp.serialize();
    Utils::FileUtils::writeVectorToFile(v, "progress.prog");
    v = Utils::FileUtils::readFileToVector("progress.prog");
    DownloadProgress dp2(v);
}