#include"Contract.h"

Contract::Contract(Money wage, int years) : wage(wage), yearsRemaining(years){}

//yýllýk maaþý döner
Money Contract::getWage() const {
	return wage;
}
//sözleþme yýlýný 1 yýl azaltýr ( sezon sonu )
void Contract::advanceYear() {
	--yearsRemaining;
}
//kontrat bittiyse true döner
bool Contract::isExpired() const {
	if (yearsRemaining == 0) {
		return 1;
	}
	return 0;
}