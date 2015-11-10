#!/bin/bash

#cache_size="500 1000 2000 3000 4000 5000"
cache_size="1000"
#video_rate="100 300 500 1000 2000 4000"
video_rate="500"
#common_rate="100 300 500 1000 2000 4000"
common_rate="100"
#error_rate=" 0.1 1.0 3.0"
error_rate="1.0"
seed_num="1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20"
time_num="30"

pushd ../../../main
make -j8
popd

mkdir config

i=1
for cache in ${cache_size};do
  for video in ${video_rate};do
    for common in ${common_rate};do
      for error in ${error_rate};do
        for seed in ${seed_num}; do
          for time in ${time_num}; do
            cp tree.config config/treeDEFAULT${i}.config
            sed -i -e "s/CACHE_SIZE 1000/CACHE_SIZE $cache  /" config/treeDEFAULT${i}.config
            sed -i -e "s/VIDEO_RATE 500/VIDEO_RATE $video  /" config/treeDEFAULT${i}.config
            sed -i -e "s/COMMON_RATE 500/COMMON_RATE $common  /" config/treeDEFAULT${i}.config
            sed -i -e "s/ERROR_RATE 0.3/ERROR_RATE $error  /" config/treeDEFAULT${i}.config
            sed -i -e "s/SEED 1/SEED $seed  /" config/treeDEFAULT${i}.config
            sed -i -e "s/SIMULATION-TIME 30S/SIMULATION-TIME $time\S  /" config/treeDEFAULT${i}.config

            cp config/treeDEFAULT${i}.config config/treeNO_VIDEO${i}.config
            sed -i -e "s/CCN_METHOD DEFAULT/CCN_METHOD NO_VIDEO/" config/treeNO_VIDEO${i}.config
            cp config/treeDEFAULT${i}.config config/treeDEFAULT_FAST${i}.config
            sed -i -e "s/CCN_METHOD DEFAULT/CCN_METHOD DEFAULT_FAST/" config/treeDEFAULT_FAST${i}.config
            cp config/treeDEFAULT${i}.config config/treePRIOR${i}.config
            sed -i -e "s/CCN_METHOD DEFAULT/CCN_METHOD PRIOR/" config/treePRIOR${i}.config
            cp config/treeDEFAULT${i}.config config/treePROPOSAL${i}.config
            sed -i -e "s/CCN_METHOD DEFAULT/CCN_METHOD PROPOSAL/" config/treePROPOSAL${i}.config

            qualnet config/treeDEFAULT_FAST${i}.config
            qualnet config/treeNO_VIDEO${i}.config
            qualnet config/treeDEFAULT${i}.config
            qualnet config/treePRIOR${i}.config
            qualnet config/treePROPOSAL${i}.config
            let i=${i}+1
          done
        done
      done
    done
  done
done
