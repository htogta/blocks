test: test-open-new test-open-invalid test-append-read test-count-endian test-update-clear test-block-new test-short-last test-from-file test-clean test-merge test-fail-reason test-insert

@test-open-new:
  cc tests/test-open-new.c -o tests/test-open-new
  ./tests/test-open-new
  rm tests/test-open-new
  
@test-open-invalid:
  cc tests/test-open-invalid.c -o tests/test-open-invalid
  ./tests/test-open-invalid
  rm tests/test-open-invalid
  
@test-append-read:
  cc tests/test-append-read.c -o tests/test-append-read
  ./tests/test-append-read
  rm tests/test-append-read
  
@test-count-endian:
  cc tests/test-count-endian.c -o tests/test-count-endian
  ./tests/test-count-endian
  rm tests/test-count-endian
  
@test-update-clear:
  cc tests/test-update-clear.c -o tests/test-update-clear
  ./tests/test-update-clear
  rm tests/test-update-clear
  
@test-block-new:
  cc tests/test-block-new.c -o tests/test-block-new
  ./tests/test-block-new
  rm tests/test-block-new
  
@test-short-last:
  cc tests/test-short-last.c -o tests/test-short-last
  ./tests/test-short-last
  rm tests/test-short-last
  
@test-from-file:
  cc tests/test-from-file.c -o tests/test-from-file
  ./tests/test-from-file
  rm tests/test-from-file
  
@test-clean:
  cc tests/test-clean.c -o tests/test-clean
  ./tests/test-clean
  rm tests/test-clean
  
@test-merge:
  cc tests/test-merge.c -o tests/test-merge
  ./tests/test-merge
  rm tests/test-merge
  
@test-fail-reason:
  cc tests/test-fail-reason.c -o tests/test-fail-reason
  ./tests/test-fail-reason
  rm tests/test-fail-reason
  
@test-insert:
  cc tests/test-insert.c -o tests/test-insert
  ./tests/test-insert
  rm tests/test-insert
