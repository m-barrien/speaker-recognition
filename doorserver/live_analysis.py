from time import sleep
import subprocess as sp

# Print iterations progress
def printProgressBar (iteration, total, prefix = '', suffix = '', decimals = 1, length = 100, fill = '█'):
    """
    Call in a loop to create terminal progress bar
    @params:
        iteration   - Required  : current iteration (Int)
        total       - Required  : total iterations (Int)
        prefix      - Optional  : prefix string (Str)
        suffix      - Optional  : suffix string (Str)
        decimals    - Optional  : positive number of decimals in percent complete (Int)
        length      - Optional  : character length of bar (Int)
        fill        - Optional  : bar fill character (Str)
    """
    percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
    filledLength = int(length * iteration // total)
    bar = fill * filledLength + '-' * (length - filledLength)
    print('%s |%s| %s%% %s' % (prefix, bar, percent, suffix))
def refreshGUI(names,values):
    sp.call('clear',shell=True)
    print("RECONOCEDOR DE VOZ")
    if len(names) != len(values):
        exit(1)
    for indx in range(len(names)):
        printProgressBar(values[indx], 1, prefix = 'Confianza:', suffix = names[indx], length = 50)


# 
# Sample Usage
# 


# A List of Items
items = list(range(0, 57))
l = len(items)

for i, item in enumerate(items):
    # Do stuff...
    sleep(0.1)
    refreshGUI(["a","b"],[0.34+i*0.01,0.2+i*0.01])
