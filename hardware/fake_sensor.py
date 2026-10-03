import sys
from main import main
if __name__ == "__main__":
    sys.argv.insert(1, "--fake")
    main()
