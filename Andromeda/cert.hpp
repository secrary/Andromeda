#pragma once

#include "utils.hpp"

// apt install libssl-dev
#include <openssl/pkcs7.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

namespace andromeda
{
    class certificate
    {
        bool is_cert = false;

        std::shared_ptr<char> root_certificate{};
        std::shared_ptr<char> creation_date{};
        std::shared_ptr<char> revoke_date{};

    public:
        
        bool is_certificate() const
        {
            return is_cert;
        }
        
        explicit certificate(const std::string& cert_dir)
        {
            if (!fs::exists(cert_dir))
            {
                return;
            }

			for (auto& p : fs::directory_iterator(cert_dir))
			{
				const auto& file_path = p.path();
				if (
                    file_path.extension() == ".RSA" ||
                    file_path.extension() == ".EC" ||
                    file_path.extension() == ".DSA"
                )
				{
                    FILE *fp = nullptr;
                    if (!(fp = fopen(file_path.string().c_str(), "rb")))
                    {
                        continue;
                    }
                    const auto pkcs7_certs = d2i_PKCS7_fp(fp, NULL);
                    fclose(fp);

                    const auto i = OBJ_obj2nid(pkcs7_certs->type);
                    STACK_OF(X509) *certs;
                    if(i == NID_pkcs7_signed) {
                        certs = pkcs7_certs->d.sign->cert;
                    } else if(i == NID_pkcs7_signedAndEnveloped) {
                        certs = pkcs7_certs->d.signed_and_enveloped->cert;
                    }

                    const auto number_of_certs = sk_X509_num(certs);
                    if (number_of_certs == 0)
                    {
                        return;
                    }
                    
                    auto root_cert = sk_X509_value(certs, number_of_certs - 1);
                    const auto end_date = X509_get_notAfter(root_cert);
                    const auto start_date = X509_get_notBefore(root_cert);

                    auto x509Bio = BIO_new(BIO_s_mem());
                    X509_print(x509Bio, root_cert);
                    auto buf_len = BIO_number_written(x509Bio);
                    root_certificate = std::shared_ptr<char>{ new char[buf_len + 1]() };
                    BIO_read(x509Bio, root_certificate.get(), buf_len + 1);
                    BIO_free(x509Bio);

                    auto start_bio = BIO_new(BIO_s_mem());
                    ASN1_TIME_print(start_bio, start_date);
                    buf_len = BIO_number_written(start_bio);
                    creation_date = std::shared_ptr<char>{ new char[buf_len + 1]() };
                    BIO_read(start_bio, creation_date.get(), buf_len + 1);
                    BIO_free(start_bio);

                    auto end_bio = BIO_new(BIO_s_mem());
                    ASN1_TIME_print(end_bio, end_date);
                    buf_len = BIO_number_written(end_bio);
                    revoke_date = std::shared_ptr<char>{ new char[buf_len + 1]() };
                    BIO_read(end_bio, revoke_date.get(), buf_len + 1);
                    BIO_free(end_bio);

                    X509_free(root_cert);
                    PKCS7_free(pkcs7_certs);

                    is_cert = true;
                    break;
				}
			}

        }

        certificate(const certificate&) = default;
		certificate& operator=(const certificate&) = default;
        
        std::shared_ptr<char> get_certificate() const 
        {
            return root_certificate;
        }

        std::shared_ptr<char> get_creation_date() const
        {
            return creation_date;
        }

        
        std::shared_ptr<char> get_revoke_date() const
        {
            return revoke_date;
        }


    };

} // namespace: andomeda