#ifndef COMMUNICATION_H
#define	COMMUNICATION_H

void AnalyzeCommandFromPi (void);
void SelectActionFromPi (void);
void SendDone(void);
void SendFailAX12(void);


void DetectSick(int channel);
void ReleaseSick (int channel);

#endif	/* COMMUNICATION_H */

