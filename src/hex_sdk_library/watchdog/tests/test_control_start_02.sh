
# Include current directory in path so we can find daemonstart
PATH=".:$PATH"
export PATH

./$TEST
