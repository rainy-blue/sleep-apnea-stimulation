# sleep-apnea-stimulation
BME349 Instrumentation Design Project.  
Electrical stimulation device designed for treatment of sleep apnea.  

Light absorption data from finger-worn pulse oximeter is relayed to necklace device which delivers transcutaneous electrical stimulation via electrodes to the user's submental throat region to contract collapsing soft airway muscles and prevent apnea episodes. Electrical stimulation is delivered in gradual increments so as to not abruptly wake the user and are calibrated to user's specific settings. The two components of the device communicate via Bluetooth.   

![image](https://user-images.githubusercontent.com/47039448/138573077-57ea8b9f-8cfb-4d67-b0d6-b0e58cd31f1a.png)
![image](https://user-images.githubusercontent.com/47039448/138573093-8f88a64a-9b9b-4765-aba1-5917bc73f3a4.png)
  
CAD Models designed by Sam Bello  

*Calibration*  
Threshold values are calibrated to be user-specific upon setup. While there is a baseline stimulation value for average adults, these vary due to unique skin impedance, skin condition, BMI, upper airway shape, and neck circumference. Additionally, oxygen absorption values detected by the pulse oximeter sensor can vary based on skin pigmentation, deoxyhemoglobin, and nail polish or dyes. 

*Safety*  
Our device operates at high voltage (still only ~3 V) but low current. The path of the current is non-lethal and the duration of stimulation is designed to minimize possibilty of any nerve damage. Research finds that skin resistance in the submental region is about 360 ohms for the average adult and can range up to 1000 ohms. Studies have also shown that a current up to 10 mA can be safely used without waking up a subject. 

![image](https://user-images.githubusercontent.com/47039448/138574734-e83fff5c-c031-42eb-b266-f295b4094985.png)

Sources:  
https://www.sciencedirect.com/science/article/abs/pii/0034568795000112  
https://www-ncbi-nlm-nih-gov.ezproxy.lib.utexas.edu/pmc/articles/PMC3996303/  
https://pubmed.ncbi.nlm.nih.gov/21454399/  
https://www-ncbi-nlm-nih-gov.ezproxy.lib.utexas.edu/pmc/articles/PMC2763825/  

Group 13 Team Members:   
Samuel Bello  
Nahom Berhane  
Grace Hu  
Novera Khan  
Kevin Wu  
