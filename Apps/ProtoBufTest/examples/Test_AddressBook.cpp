#include <iostream>
#include <fstream>
#include <string>
#include "addressbook.pb.h"

// Refer to https://developers.google.com/protocol-buffers/docs/cpptutorial

using namespace std;

// This function fills in a Person message based on user input.
void PromptForAddress(tutorial::Person* person) {
    cout << "Enter person ID number: ";
    int id;
    id = 1078212; //cin >> id;
    person->set_id(id);
    // cin.ignore(256, '\n');

    cout << "Enter name: ";
    *person->mutable_name() = "Name1"; // getline(cin, *person->mutable_name());

    cout << "Enter email address (blank for none): ";
    string email;
    email = "email1@sap.com"; // getline(cin, email);
    if (!email.empty()) {
        person->set_email(email);
    }

    int count = 3;
    while (count-- > 0) {
        cout << "Enter a phone number (or leave blank to finish): ";
        string number;
        number = "555-555-5555"; // getline(cin, number);
        if (number.empty()) {
            break;
        }

        tutorial::Person::PhoneNumber* phone_number = person->add_phone();
        phone_number->set_number(number);

        cout << "Is this a mobile, home, or work phone? ";
        string type;
        type = "mobile"; // getline(cin, type);
        if (type == "mobile") {
            phone_number->set_type(tutorial::Person::MOBILE);
        } else if (type == "home") {
            phone_number->set_type(tutorial::Person::HOME);
        } else if (type == "work") {
            phone_number->set_type(tutorial::Person::WORK);
        } else {
            cout << "Unknown phone type.  Using default." << endl;
        }
    }
}


// Iterates though all people in the AddressBook and prints info about them.
void ListPeople(const tutorial::AddressBook& address_book) {
    for (int i = 0; i < address_book.person_size(); i++) {
        const tutorial::Person& person = address_book.person(i);

        cout << "Person ID: " << person.id() << endl;
        cout << "  Name: " << person.name() << endl;
        if (person.has_email()) {
            cout << "  E-mail address: " << person.email() << endl;
        }

        for (int j = 0; j < person.phone_size(); j++) {
            const tutorial::Person::PhoneNumber& phone_number = person.phone(j);

            switch (phone_number.type()) {
            case tutorial::Person::MOBILE:
                cout << "  Mobile phone #: ";
                break;
            case tutorial::Person::HOME:
                cout << "  Home phone #: ";
                break;
            case tutorial::Person::WORK:
                cout << "  Work phone #: ";
                break;
            }
            cout << phone_number.number() << endl;
        }
    }
}

int Test_AddressBook() {
    const char FILE_NAME[] = "Temp/AddressBook.data";
    tutorial::AddressBook address_book;

    {
        // Read the existing address book.
        fstream input(FILE_NAME, ios::in | ios::binary);
        if (!input) {
            cout << FILE_NAME << ": File not found.  Creating a new file." << endl;
        } else if (!address_book.ParseFromIstream(&input)) {
            cerr << "Failed to parse address book." << endl;
            return -1;
        }
    }

    // Add an address.
    PromptForAddress(address_book.add_person());

    {
        // Write the new address book back to disk.
        fstream output(FILE_NAME, ios::out | ios::trunc | ios::binary);
        if (!address_book.SerializeToOstream(&output)) {
            cerr << "Failed to write address book." << endl;
            return -1;
        }
    }

    ListPeople(address_book);

    return 0;
}
