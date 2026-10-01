test: test-open-new test-open-invalid test-update-clear test-block-new test-from-file

@test-open-new:
  cc tests/test-open-new.c -o tests/test-open-new
  ./tests/test-open-new
  rm tests/test-open-new
  
@test-open-invalid:
  cc tests/test-open-invalid.c -o tests/test-open-invalid
  ./tests/test-open-invalid
  rm tests/test-open-invalid
  
@test-update-clear:
  cc tests/test-update-clear.c -o tests/test-update-clear
  ./tests/test-update-clear
  rm tests/test-update-clear
  
@test-block-new:
  cc tests/test-block-new.c -o tests/test-block-new
  ./tests/test-block-new
  rm tests/test-block-new
    
@test-from-file:
  cc tests/test-from-file.c -o tests/test-from-file
  ./tests/test-from-file
  rm tests/test-from-file
