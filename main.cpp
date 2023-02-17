#include <iostream>
#include <occi.h>

using oracle::occi::Environment;
using oracle::occi::Connection;

using namespace oracle::occi;
using namespace std;

struct ShoppingCart
{
	int product_id;
	double price;
	int quantity;
};

int mainMenu()
{
	int menuSelect;
	cout << "******************** Main Menu ********************" << endl
		<< "1)	Login" << endl
		<< "0)	Exit" << endl
		<< "Enter an option (0-1): ";
	cin >> menuSelect;
	while ((menuSelect != 0 && menuSelect != 1) || cin.fail())
	{
		cin.clear();
		cin.ignore(365, '\n');
		cout << "******************** Main Menu ********************" << endl
			<< "1)	Login" << endl
			<< "0)	Exit" << endl
			<< "You entered a wrong value. Enter an option (0-1):";
		cin >> menuSelect;
	}
	return menuSelect;
}

int customerLogin(Connection* conn, int customerId)
{
	try
	{
		Statement* stmt = conn->createStatement();
		stmt->setSQL("begin find_customer(:1, :2); end;");
		stmt->setInt(1, customerId);
		int found;
		stmt->registerOutParam(2, Type::OCCIINT, sizeof(found));
		stmt->executeUpdate();
		found = stmt->getInt(2);
		conn->terminateStatement(stmt);
		return found;
	}
	catch (SQLException & sqlExcp)
	{
		cout << sqlExcp.getErrorCode() << ": " << sqlExcp.getMessage();
	}
}

double findProduct(Connection* conn, int product_id)
{
	try
	{
		Statement* stmt = conn->createStatement();
		stmt->setSQL("begin find_product(:1, :2); end;");
		stmt->setInt(1, product_id);
		double price;
		stmt->registerOutParam(2, Type::OCCIDOUBLE, sizeof(price));
		stmt->executeUpdate();
		price = stmt->getDouble(2);
		conn->terminateStatement(stmt);
		return price;
	}
	catch (SQLException & sqlExcp)
	{
		cout << sqlExcp.getErrorCode() << ": " << sqlExcp.getMessage();
	}
}


int addToCart(Connection* conn, struct ShoppingCart cart[])
{
	int counter = 0, choise = 0;
	bool more = true;
	double productFound = 1.1;
	cout << "-------------- Add Products to Cart --------------" << endl;
	while(counter < 5 && more)
	{
		do
		{
			cout << "Enter the product ID: ";
			cin >> cart[counter].product_id;
			while (cin.fail())
			{
				cin.clear();
				cin.ignore(365, '\n');
				cout << "You must enter an integer! Enter the product ID again: ";
				cin >> cart[counter].product_id;
			}
			productFound = findProduct(conn, cart[counter].product_id);
			if(productFound == 0)
			{
				cout << "The product does not exists. Try again..." << endl;
			}
			else
			{
				cart[counter].price = productFound;
				cout << "Product Price: " << cart[counter].price << endl;
				cout << "Enter the product Quantity: ";
				cin >> cart[counter].quantity;
				while (cin.fail())
				{
					cin.clear();
					cin.ignore(365, '\n');
					cout << "You must enter an integer! Please enter a quantity again: ";
					cin >> cart[counter].quantity;
				}
			}
		} while (productFound == 0);
		cout << "Enter 1 to add more products or 0 to checkout: ";
		cin >> choise;
		while (cin.fail() || (choise != 1 && choise != 0))
		{
			cin.clear();
			cin.ignore(365, '\n');
			cout << "Wrong value! Enter your option again: ";
			cin >> choise;
		}
		if (choise == 0)
		{
			more = false;
		}
		else
		{
			counter++;
		}
	}
	return counter + 1;
}

void displayProducts(struct ShoppingCart cart[], int productCount)
{
	double total = 0.0;
	cout << "------- Ordered Products ---------" << endl;
	for (auto i = 0; i < productCount; i++)
	{
		cout << "---Item " << i + 1 << endl
			<< "Product ID: " << cart[i].product_id << endl
			<< "Price: " << cart[i].price << endl
			<< "Quantity: " << cart[i].quantity << endl;
		total += cart[i].quantity * cart[i].price;
	}
	cout << "----------------------------------" << endl
		<< "Total: " << total << endl;
}

int checkout(Connection* conn, struct ShoppingCart cart[], int customerId, int productCount)
{
	string choise;
	int new_order_id, again;
	bool loop = true;
	cout << "Would you like to checkout ? (Y / y or N / n) ";
	cin >> choise;
	while (loop)
	{
		if (choise == "Y" || choise == "y")
		{
			try
			{
				Statement* stmt = conn->createStatement();
				stmt->setSQL("begin add_order(:1, :2); end;");
				stmt->setInt(1, customerId);
				stmt->registerOutParam(2, Type::OCCIINT, sizeof(new_order_id));
				stmt->executeUpdate();
				new_order_id = stmt->getInt(2);
				conn->terminateStatement(stmt);
			}
			catch (SQLException & sqlExcp)
			{
				cout << sqlExcp.getErrorCode() << ": " << sqlExcp.getMessage();
			}
			for (int i = 0; i < productCount; i++)
			{
				try
				{
					Statement* stmt = conn->createStatement();
					stmt->setSQL("begin add_order_item(:1, :2, :3, :4, :5); end;");
					stmt->setInt(1, new_order_id);
					stmt->setInt(2, (i + 1));
					stmt->setInt(3, cart[i].product_id);
					stmt->setInt(4, cart[i].quantity);
					stmt->setDouble(5, cart[i].price);
					stmt->executeUpdate();
					conn->terminateStatement(stmt);
				}
				catch (SQLException & sqlExcp)
				{
					cout << sqlExcp.getErrorCode() << ": " << sqlExcp.getMessage();
				}
			}
			cout << "The order is successfully completed." << endl;
			again = 1;
			loop = false;
		}
		else if (choise == "N" || choise == "n")
		{
			cout << "The order is cancelled." << endl;
			again = 0;
			loop = false;
		}
		else
		{
			cout << "Please enter only Y/y or N/n: ";
			cin >> choise;
		}
	}
	return again;
}

int main(void)
{
	Environment* env = nullptr;
	Connection* conn = nullptr;
	string str;
	string user = "";
	string pw = "";
	string constr = "myoracle12c.senecacollege.ca:1521/oracle12c";
	try
	{
		env = Environment::createEnvironment(Environment::DEFAULT);
		conn = env->createConnection(user, pw, constr);
		cout << "Connection is Successful!" << endl;
		int customer_id;
		bool loop = true;
		int selection = 1;
		while (loop)
		{
			selection = mainMenu();
			if (selection == 1)
			{
				cout << "Enter the customer ID: ";
				while (!(cin >> customer_id))
				{
					cout << "You must enter an integer!" << endl;
					cin.clear();
					cin.ignore(365, '\n');
					cout << "Enter the customer ID: ";
				}
				if (customerLogin(conn, customer_id) == 1)
				{
					ShoppingCart cart[5];
					int count = addToCart(conn, cart);
					displayProducts(cart, count);
					int again = checkout(conn, cart, customer_id, count);
				}
				else
				{
					cout << "The customer does not exist." << endl;
				}
			}
			if (selection == 0)
			{
				cout << "Good bye!..." << endl;
				loop = false;
			}			
		}
		env->terminateConnection(conn);
		Environment::terminateEnvironment(env);
	}
	catch (SQLException & sqlExcp)
	{
		cout << sqlExcp.getErrorCode() << ": " << sqlExcp.getMessage();
	}
	return 0;
}
